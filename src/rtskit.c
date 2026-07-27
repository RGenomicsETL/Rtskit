#include <R.h>
#include <R_ext/Boolean.h>
#include <Rinternals.h>

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <tskit.h>

typedef struct {
    tsk_treeseq_t value;
    int initialized;
} rtskit_treeseq_handle;

static SEXP rtskit_handle_tag = NULL;

void
rtskit_native_bug(const char *file, int line, const char *message)
{
    Rf_error(
        "internal tskit invariant failed in %s at line %d: %s", file, line, message);
}

static void
rtskit_error(int code, const char *operation)
{
    Rf_error("%s: %s", operation, tsk_strerror(code));
}

static void
rtskit_finalizer(SEXP ptr)
{
    rtskit_treeseq_handle *handle
        = (rtskit_treeseq_handle *) R_ExternalPtrAddr(ptr);
    if (handle != NULL) {
        if (handle->initialized) {
            tsk_treeseq_free(&handle->value);
            handle->initialized = 0;
        }
        free(handle);
        R_ClearExternalPtr(ptr);
    }
}

static rtskit_treeseq_handle *
rtskit_get_handle(SEXP ptr)
{
    rtskit_treeseq_handle *handle;
    if (TYPEOF(ptr) != EXTPTRSXP) {
        Rf_error("tree sequence pointer must be an external pointer");
    }
    if (rtskit_handle_tag != NULL && R_ExternalPtrTag(ptr) != rtskit_handle_tag) {
        Rf_error("external pointer is not an Rtskit tree sequence");
    }
    handle = (rtskit_treeseq_handle *) R_ExternalPtrAddr(ptr);
    if (handle == NULL || !handle->initialized) {
        Rf_error("tree sequence pointer is no longer valid");
    }
    return handle;
}

static SEXP
rtskit_wrap_handle(rtskit_treeseq_handle *handle)
{
    SEXP ptr = PROTECT(R_MakeExternalPtr(handle, rtskit_handle_tag, R_NilValue));
    R_RegisterCFinalizerEx(ptr, rtskit_finalizer, TRUE);
    UNPROTECT(1);
    return ptr;
}

static const char *
rtskit_scalar_string(SEXP value, const char *argument)
{
    if (TYPEOF(value) != STRSXP || XLENGTH(value) != 1
        || STRING_ELT(value, 0) == NA_STRING) {
        Rf_error("%s must be one non-missing string", argument);
    }
    return CHAR(STRING_ELT(value, 0));
}

static void
rtskit_check_rows(tsk_size_t rows, const char *table)
{
    if (rows > (tsk_size_t) INT_MAX) {
        Rf_error("%s has more rows than this R data-frame interface supports", table);
    }
}

static void
rtskit_set_names(SEXP object, const char **names, int length)
{
    SEXP result_names = PROTECT(Rf_allocVector(STRSXP, length));
    for (int j = 0; j < length; j++) {
        SET_STRING_ELT(result_names, j, Rf_mkChar(names[j]));
    }
    Rf_setAttrib(object, R_NamesSymbol, result_names);
    UNPROTECT(1);
}

static void
rtskit_set_data_frame(SEXP object, int rows)
{
    SEXP row_names = PROTECT(Rf_allocVector(INTSXP, 2));
    INTEGER(row_names)[0] = NA_INTEGER;
    INTEGER(row_names)[1] = -rows;
    Rf_setAttrib(object, R_RowNamesSymbol, row_names);
    Rf_setAttrib(object, R_ClassSymbol, Rf_mkString("data.frame"));
    UNPROTECT(1);
}

SEXP
RC_rtskit_initialize(SEXP package_name)
{
    const char *name = rtskit_scalar_string(package_name, "package_name");
    rtskit_handle_tag = Rf_install(name);
    return R_NilValue;
}

SEXP
RC_rtskit_is_valid(SEXP ptr)
{
    rtskit_treeseq_handle *handle;
    if (TYPEOF(ptr) != EXTPTRSXP) {
        return Rf_ScalarLogical(FALSE);
    }
    if (rtskit_handle_tag != NULL && R_ExternalPtrTag(ptr) != rtskit_handle_tag) {
        return Rf_ScalarLogical(FALSE);
    }
    handle = (rtskit_treeseq_handle *) R_ExternalPtrAddr(ptr);
    return Rf_ScalarLogical(handle != NULL && handle->initialized);
}

SEXP
RC_rtskit_load(SEXP path)
{
    const char *filename = rtskit_scalar_string(path, "path");
    rtskit_treeseq_handle *handle
        = (rtskit_treeseq_handle *) calloc(1, sizeof(*handle));
    int ret;
    if (handle == NULL) {
        Rf_error("could not allocate a tree sequence handle");
    }
    ret = tsk_treeseq_load(&handle->value, filename, 0);
    if (ret < 0) {
        tsk_treeseq_free(&handle->value);
        free(handle);
        rtskit_error(ret, "could not load tree sequence");
    }
    handle->initialized = 1;
    return rtskit_wrap_handle(handle);
}

SEXP
RC_rtskit_dump(SEXP ptr, SEXP path)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const char *filename = rtskit_scalar_string(path, "path");
    int ret = tsk_treeseq_dump(&handle->value, filename, 0);
    if (ret < 0) {
        rtskit_error(ret, "could not write tree sequence");
    }
    return path;
}

SEXP
RC_rtskit_summary(SEXP ptr)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const tsk_treeseq_t *ts = &handle->value;
    const char *names[] = { "sequence_length", "trees", "samples", "individuals",
        "nodes", "edges", "sites", "mutations", "populations", "migrations",
        "provenances" };
    SEXP result = PROTECT(Rf_allocVector(VECSXP, 11));

    SET_VECTOR_ELT(result, 0, Rf_ScalarReal(tsk_treeseq_get_sequence_length(ts)));
    SET_VECTOR_ELT(result, 1, Rf_ScalarReal((double) tsk_treeseq_get_num_trees(ts)));
    SET_VECTOR_ELT(result, 2, Rf_ScalarReal((double) tsk_treeseq_get_num_samples(ts)));
    SET_VECTOR_ELT(result, 3,
        Rf_ScalarReal((double) tsk_treeseq_get_num_individuals(ts)));
    SET_VECTOR_ELT(result, 4, Rf_ScalarReal((double) tsk_treeseq_get_num_nodes(ts)));
    SET_VECTOR_ELT(result, 5, Rf_ScalarReal((double) tsk_treeseq_get_num_edges(ts)));
    SET_VECTOR_ELT(result, 6, Rf_ScalarReal((double) tsk_treeseq_get_num_sites(ts)));
    SET_VECTOR_ELT(result, 7,
        Rf_ScalarReal((double) tsk_treeseq_get_num_mutations(ts)));
    SET_VECTOR_ELT(result, 8,
        Rf_ScalarReal((double) tsk_treeseq_get_num_populations(ts)));
    SET_VECTOR_ELT(result, 9,
        Rf_ScalarReal((double) tsk_treeseq_get_num_migrations(ts)));
    SET_VECTOR_ELT(result, 10,
        Rf_ScalarReal((double) tsk_treeseq_get_num_provenances(ts)));
    rtskit_set_names(result, names, 11);
    UNPROTECT(1);
    return result;
}

SEXP
RC_rtskit_samples(SEXP ptr)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const tsk_treeseq_t *ts = &handle->value;
    tsk_size_t count = tsk_treeseq_get_num_samples(ts);
    const tsk_id_t *samples = tsk_treeseq_get_samples(ts);
    rtskit_check_rows(count, "sample list");
    SEXP result = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t) count));
    for (tsk_size_t j = 0; j < count; j++) {
        INTEGER(result)[j] = samples[j];
    }
    UNPROTECT(1);
    return result;
}

SEXP
RC_rtskit_nodes(SEXP ptr)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const tsk_node_table_t *nodes = &handle->value.tables->nodes;
    const char *names[] = { "id", "flags", "time", "population", "individual" };
    int rows;
    rtskit_check_rows(nodes->num_rows, "node table");
    rows = (int) nodes->num_rows;

    SEXP result = PROTECT(Rf_allocVector(VECSXP, 5));
    SEXP id = PROTECT(Rf_allocVector(INTSXP, rows));
    SEXP flags = PROTECT(Rf_allocVector(REALSXP, rows));
    SEXP time = PROTECT(Rf_allocVector(REALSXP, rows));
    SEXP population = PROTECT(Rf_allocVector(INTSXP, rows));
    SEXP individual = PROTECT(Rf_allocVector(INTSXP, rows));

    for (int j = 0; j < rows; j++) {
        INTEGER(id)[j] = j;
        REAL(flags)[j] = (double) nodes->flags[j];
        REAL(time)[j] = nodes->time[j];
        INTEGER(population)[j] = nodes->population[j];
        INTEGER(individual)[j] = nodes->individual[j];
    }
    SET_VECTOR_ELT(result, 0, id);
    SET_VECTOR_ELT(result, 1, flags);
    SET_VECTOR_ELT(result, 2, time);
    SET_VECTOR_ELT(result, 3, population);
    SET_VECTOR_ELT(result, 4, individual);
    rtskit_set_names(result, names, 5);
    rtskit_set_data_frame(result, rows);
    UNPROTECT(6);
    return result;
}

SEXP
RC_rtskit_edges(SEXP ptr)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const tsk_edge_table_t *edges = &handle->value.tables->edges;
    const char *names[] = { "id", "left", "right", "parent", "child" };
    int rows;
    rtskit_check_rows(edges->num_rows, "edge table");
    rows = (int) edges->num_rows;

    SEXP result = PROTECT(Rf_allocVector(VECSXP, 5));
    SEXP id = PROTECT(Rf_allocVector(INTSXP, rows));
    SEXP left = PROTECT(Rf_allocVector(REALSXP, rows));
    SEXP right = PROTECT(Rf_allocVector(REALSXP, rows));
    SEXP parent = PROTECT(Rf_allocVector(INTSXP, rows));
    SEXP child = PROTECT(Rf_allocVector(INTSXP, rows));

    for (int j = 0; j < rows; j++) {
        INTEGER(id)[j] = j;
        REAL(left)[j] = edges->left[j];
        REAL(right)[j] = edges->right[j];
        INTEGER(parent)[j] = edges->parent[j];
        INTEGER(child)[j] = edges->child[j];
    }
    SET_VECTOR_ELT(result, 0, id);
    SET_VECTOR_ELT(result, 1, left);
    SET_VECTOR_ELT(result, 2, right);
    SET_VECTOR_ELT(result, 3, parent);
    SET_VECTOR_ELT(result, 4, child);
    rtskit_set_names(result, names, 5);
    rtskit_set_data_frame(result, rows);
    UNPROTECT(6);
    return result;
}

SEXP
RC_rtskit_trees(SEXP ptr)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const tsk_treeseq_t *ts = &handle->value;
    tsk_size_t count = tsk_treeseq_get_num_trees(ts);
    const char *names[] = { "index", "left", "right", "roots", "edges" };
    tsk_tree_t tree;
    int ret;
    int row = 0;
    rtskit_check_rows(count, "tree sequence");

    SEXP result = PROTECT(Rf_allocVector(VECSXP, 5));
    SEXP index = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t) count));
    SEXP left = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t) count));
    SEXP right = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t) count));
    SEXP roots = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t) count));
    SEXP edges = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t) count));

    ret = tsk_tree_init(&tree, ts, 0);
    if (ret < 0) {
        UNPROTECT(6);
        rtskit_error(ret, "could not initialize tree iterator");
    }
    for (ret = tsk_tree_first(&tree); ret == TSK_TREE_OK;
         ret = tsk_tree_next(&tree)) {
        if ((tsk_size_t) row >= count) {
            tsk_tree_free(&tree);
            UNPROTECT(6);
            Rf_error("tree iterator exceeded declared tree count");
        }
        INTEGER(index)[row] = tree.index;
        REAL(left)[row] = tree.interval.left;
        REAL(right)[row] = tree.interval.right;
        REAL(roots)[row] = (double) tsk_tree_get_num_roots(&tree);
        REAL(edges)[row] = (double) tree.num_edges;
        row++;
    }
    tsk_tree_free(&tree);
    if (ret < 0) {
        UNPROTECT(6);
        rtskit_error(ret, "tree iteration failed");
    }
    if ((tsk_size_t) row != count) {
        UNPROTECT(6);
        Rf_error("tree iterator returned an unexpected number of trees");
    }

    SET_VECTOR_ELT(result, 0, index);
    SET_VECTOR_ELT(result, 1, left);
    SET_VECTOR_ELT(result, 2, right);
    SET_VECTOR_ELT(result, 3, roots);
    SET_VECTOR_ELT(result, 4, edges);
    rtskit_set_names(result, names, 5);
    rtskit_set_data_frame(result, row);
    UNPROTECT(6);
    return result;
}

SEXP
RC_rtskit_example(void)
{
    rtskit_treeseq_handle *handle
        = (rtskit_treeseq_handle *) calloc(1, sizeof(*handle));
    tsk_table_collection_t tables;
    int ret;
    int tables_initialized = 0;

    if (handle == NULL) {
        Rf_error("could not allocate a tree sequence handle");
    }
    ret = tsk_table_collection_init(&tables, 0);
    if (ret < 0) {
        free(handle);
        rtskit_error(ret, "could not initialize example tables");
    }
    tables_initialized = 1;
    tables.sequence_length = 10.0;

    ret = tsk_node_table_add_row(
        &tables.nodes, TSK_NODE_IS_SAMPLE, 0.0, TSK_NULL, TSK_NULL, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_node_table_add_row(
        &tables.nodes, TSK_NODE_IS_SAMPLE, 0.0, TSK_NULL, TSK_NULL, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_node_table_add_row(
        &tables.nodes, 0, 1.0, TSK_NULL, TSK_NULL, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_edge_table_add_row(&tables.edges, 0.0, 10.0, 2, 0, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_edge_table_add_row(&tables.edges, 0.0, 10.0, 2, 1, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_site_table_add_row(&tables.sites, 4.0, "0", 1, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_mutation_table_add_row(
        &tables.mutations, 0, 0, TSK_NULL, TSK_UNKNOWN_TIME, "1", 1, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_table_collection_sort(&tables, NULL, 0);
    if (ret < 0) goto example_error;

    ret = tsk_treeseq_init(&handle->value, &tables, TSK_TS_INIT_BUILD_INDEXES);
    tsk_table_collection_free(&tables);
    tables_initialized = 0;
    if (ret < 0) {
        tsk_treeseq_free(&handle->value);
        free(handle);
        rtskit_error(ret, "could not initialize example tree sequence");
    }
    handle->initialized = 1;
    return rtskit_wrap_handle(handle);

example_error:
    if (tables_initialized) {
        tsk_table_collection_free(&tables);
    }
    free(handle);
    rtskit_error(ret, "could not build example tree sequence");
    return R_NilValue;
}
