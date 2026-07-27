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

typedef enum {
    RTSKIT_TABLE_TREE_SEQUENCE,
    RTSKIT_TABLE_REFERENCE_SEQUENCE,
    RTSKIT_TABLE_INDIVIDUALS,
    RTSKIT_TABLE_NODES,
    RTSKIT_TABLE_EDGES,
    RTSKIT_TABLE_MIGRATIONS,
    RTSKIT_TABLE_SITES,
    RTSKIT_TABLE_MUTATIONS,
    RTSKIT_TABLE_POPULATIONS
} rtskit_table_kind;

static rtskit_table_kind
rtskit_parse_table(const char *table)
{
    if (strcmp(table, "tree_sequence") == 0) return RTSKIT_TABLE_TREE_SEQUENCE;
    if (strcmp(table, "reference_sequence") == 0)
        return RTSKIT_TABLE_REFERENCE_SEQUENCE;
    if (strcmp(table, "individuals") == 0) return RTSKIT_TABLE_INDIVIDUALS;
    if (strcmp(table, "nodes") == 0) return RTSKIT_TABLE_NODES;
    if (strcmp(table, "edges") == 0) return RTSKIT_TABLE_EDGES;
    if (strcmp(table, "migrations") == 0) return RTSKIT_TABLE_MIGRATIONS;
    if (strcmp(table, "sites") == 0) return RTSKIT_TABLE_SITES;
    if (strcmp(table, "mutations") == 0) return RTSKIT_TABLE_MUTATIONS;
    if (strcmp(table, "populations") == 0) return RTSKIT_TABLE_POPULATIONS;
    Rf_error("unsupported metadata table: %s", table);
    return RTSKIT_TABLE_TREE_SEQUENCE;
}

static SEXP
rtskit_raw_bytes(const char *data, tsk_size_t length)
{
    if (length > (tsk_size_t) R_XLEN_T_MAX) {
        Rf_error("metadata value is too large for an R raw vector");
    }
    SEXP result = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t) length));
    if (length > 0) {
        memcpy(RAW(result), data, length);
    }
    UNPROTECT(1);
    return result;
}

static tsk_size_t
rtskit_metadata_rows(const tsk_table_collection_t *tables, rtskit_table_kind table)
{
    switch (table) {
    case RTSKIT_TABLE_INDIVIDUALS:
        return tables->individuals.num_rows;
    case RTSKIT_TABLE_NODES:
        return tables->nodes.num_rows;
    case RTSKIT_TABLE_EDGES:
        return tables->edges.num_rows;
    case RTSKIT_TABLE_MIGRATIONS:
        return tables->migrations.num_rows;
    case RTSKIT_TABLE_SITES:
        return tables->sites.num_rows;
    case RTSKIT_TABLE_MUTATIONS:
        return tables->mutations.num_rows;
    case RTSKIT_TABLE_POPULATIONS:
        return tables->populations.num_rows;
    default:
        return 0;
    }
}

static void
rtskit_metadata_row(const tsk_table_collection_t *tables, rtskit_table_kind table,
    tsk_id_t id, const char **data, tsk_size_t *length)
{
    const char *column = NULL;
    const tsk_size_t *offset = NULL;
    switch (table) {
    case RTSKIT_TABLE_INDIVIDUALS:
        column = tables->individuals.metadata;
        offset = tables->individuals.metadata_offset;
        break;
    case RTSKIT_TABLE_NODES:
        column = tables->nodes.metadata;
        offset = tables->nodes.metadata_offset;
        break;
    case RTSKIT_TABLE_EDGES:
        column = tables->edges.metadata;
        offset = tables->edges.metadata_offset;
        break;
    case RTSKIT_TABLE_MIGRATIONS:
        column = tables->migrations.metadata;
        offset = tables->migrations.metadata_offset;
        break;
    case RTSKIT_TABLE_SITES:
        column = tables->sites.metadata;
        offset = tables->sites.metadata_offset;
        break;
    case RTSKIT_TABLE_MUTATIONS:
        column = tables->mutations.metadata;
        offset = tables->mutations.metadata_offset;
        break;
    case RTSKIT_TABLE_POPULATIONS:
        column = tables->populations.metadata;
        offset = tables->populations.metadata_offset;
        break;
    default:
        Rf_error("metadata rows are not defined for this table");
    }
    *length = offset[id + 1] - offset[id];
    *data = *length == 0 ? NULL : column + offset[id];
}

static void
rtskit_metadata_schema_view(const tsk_table_collection_t *tables,
    rtskit_table_kind table, const char **data, tsk_size_t *length)
{
    switch (table) {
    case RTSKIT_TABLE_TREE_SEQUENCE:
        *data = tables->metadata_schema;
        *length = tables->metadata_schema_length;
        break;
    case RTSKIT_TABLE_REFERENCE_SEQUENCE:
        *data = tables->reference_sequence.metadata_schema;
        *length = tables->reference_sequence.metadata_schema_length;
        break;
    case RTSKIT_TABLE_INDIVIDUALS:
        *data = tables->individuals.metadata_schema;
        *length = tables->individuals.metadata_schema_length;
        break;
    case RTSKIT_TABLE_NODES:
        *data = tables->nodes.metadata_schema;
        *length = tables->nodes.metadata_schema_length;
        break;
    case RTSKIT_TABLE_EDGES:
        *data = tables->edges.metadata_schema;
        *length = tables->edges.metadata_schema_length;
        break;
    case RTSKIT_TABLE_MIGRATIONS:
        *data = tables->migrations.metadata_schema;
        *length = tables->migrations.metadata_schema_length;
        break;
    case RTSKIT_TABLE_SITES:
        *data = tables->sites.metadata_schema;
        *length = tables->sites.metadata_schema_length;
        break;
    case RTSKIT_TABLE_MUTATIONS:
        *data = tables->mutations.metadata_schema;
        *length = tables->mutations.metadata_schema_length;
        break;
    case RTSKIT_TABLE_POPULATIONS:
        *data = tables->populations.metadata_schema;
        *length = tables->populations.metadata_schema_length;
        break;
    }
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
RC_rtskit_populations(SEXP ptr)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const tsk_population_table_t *populations = &handle->value.tables->populations;
    const char *names[] = { "id", "metadata_length" };
    int rows;
    rtskit_check_rows(populations->num_rows, "population table");
    rows = (int) populations->num_rows;

    SEXP result = PROTECT(Rf_allocVector(VECSXP, 2));
    SEXP id = PROTECT(Rf_allocVector(INTSXP, rows));
    SEXP metadata_length = PROTECT(Rf_allocVector(REALSXP, rows));
    for (int j = 0; j < rows; j++) {
        INTEGER(id)[j] = j;
        REAL(metadata_length)[j]
            = (double) (populations->metadata_offset[j + 1]
                        - populations->metadata_offset[j]);
    }
    SET_VECTOR_ELT(result, 0, id);
    SET_VECTOR_ELT(result, 1, metadata_length);
    rtskit_set_names(result, names, 2);
    rtskit_set_data_frame(result, rows);
    UNPROTECT(3);
    return result;
}

SEXP
RC_rtskit_individuals(SEXP ptr)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const tsk_individual_table_t *individuals = &handle->value.tables->individuals;
    const char *names[] = { "id", "flags", "location", "parents", "metadata_length" };
    int rows;
    rtskit_check_rows(individuals->num_rows, "individual table");
    rows = (int) individuals->num_rows;

    SEXP result = PROTECT(Rf_allocVector(VECSXP, 5));
    SEXP id = PROTECT(Rf_allocVector(INTSXP, rows));
    SEXP flags = PROTECT(Rf_allocVector(REALSXP, rows));
    SEXP location = PROTECT(Rf_allocVector(VECSXP, rows));
    SEXP parents = PROTECT(Rf_allocVector(VECSXP, rows));
    SEXP metadata_length = PROTECT(Rf_allocVector(REALSXP, rows));

    for (int j = 0; j < rows; j++) {
        tsk_size_t location_length
            = individuals->location_offset[j + 1] - individuals->location_offset[j];
        tsk_size_t parents_length
            = individuals->parents_offset[j + 1] - individuals->parents_offset[j];
        tsk_size_t metadata_size
            = individuals->metadata_offset[j + 1] - individuals->metadata_offset[j];
        if (location_length > (tsk_size_t) R_XLEN_T_MAX
            || parents_length > (tsk_size_t) R_XLEN_T_MAX) {
            UNPROTECT(6);
            Rf_error("individual ragged column is too large for R");
        }
        SEXP row_location
            = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t) location_length));
        SEXP row_parents
            = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t) parents_length));
        for (tsk_size_t k = 0; k < location_length; k++) {
            REAL(row_location)[k]
                = individuals->location[individuals->location_offset[j] + k];
        }
        for (tsk_size_t k = 0; k < parents_length; k++) {
            INTEGER(row_parents)[k]
                = individuals->parents[individuals->parents_offset[j] + k];
        }
        INTEGER(id)[j] = j;
        REAL(flags)[j] = (double) individuals->flags[j];
        REAL(metadata_length)[j] = (double) metadata_size;
        SET_VECTOR_ELT(location, j, row_location);
        SET_VECTOR_ELT(parents, j, row_parents);
        UNPROTECT(2);
    }
    SET_VECTOR_ELT(result, 0, id);
    SET_VECTOR_ELT(result, 1, flags);
    SET_VECTOR_ELT(result, 2, location);
    SET_VECTOR_ELT(result, 3, parents);
    SET_VECTOR_ELT(result, 4, metadata_length);
    rtskit_set_names(result, names, 5);
    rtskit_set_data_frame(result, rows);
    UNPROTECT(6);
    return result;
}

SEXP
RC_rtskit_metadata(SEXP ptr, SEXP table_value, SEXP ids)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const tsk_table_collection_t *tables = handle->value.tables;
    const char *table_name = rtskit_scalar_string(table_value, "table");
    rtskit_table_kind table = rtskit_parse_table(table_name);
    const char *data = NULL;
    tsk_size_t length = 0;

    if (table == RTSKIT_TABLE_TREE_SEQUENCE
        || table == RTSKIT_TABLE_REFERENCE_SEQUENCE) {
        if (TYPEOF(ids) != INTSXP || XLENGTH(ids) != 0) {
            Rf_error("id is not defined for top-level metadata");
        }
        if (table == RTSKIT_TABLE_TREE_SEQUENCE) {
            data = tables->metadata;
            length = tables->metadata_length;
        } else {
            data = tables->reference_sequence.metadata;
            length = tables->reference_sequence.metadata_length;
        }
        return rtskit_raw_bytes(data, length);
    }

    if (TYPEOF(ids) != INTSXP) {
        Rf_error("id must be an integer vector");
    }
    tsk_size_t rows = rtskit_metadata_rows(tables, table);
    R_xlen_t count = XLENGTH(ids);
    SEXP result = PROTECT(Rf_allocVector(VECSXP, count));
    for (R_xlen_t j = 0; j < count; j++) {
        int id = INTEGER(ids)[j];
        if (id < 0 || (tsk_size_t) id >= rows) {
            UNPROTECT(1);
            Rf_error("metadata row ID is out of range for %s", table_name);
        }
        rtskit_metadata_row(tables, table, id, &data, &length);
        SET_VECTOR_ELT(result, j, rtskit_raw_bytes(data, length));
    }
    UNPROTECT(1);
    return result;
}

SEXP
RC_rtskit_metadata_schema(SEXP ptr, SEXP table_value)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const char *table_name = rtskit_scalar_string(table_value, "table");
    rtskit_table_kind table = rtskit_parse_table(table_name);
    const char *data = NULL;
    tsk_size_t length = 0;
    rtskit_metadata_schema_view(handle->value.tables, table, &data, &length);
    return rtskit_raw_bytes(data, length);
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

static tsk_id_t
rtskit_find_source_population(const tsk_tree_t *tree,
    const tsk_node_table_t *nodes, tsk_id_t sample, const unsigned char *is_source,
    tsk_size_t num_populations)
{
    tsk_id_t node = sample;
    while (node != TSK_NULL) {
        tsk_id_t population = nodes->population[node];
        if (population >= 0 && (tsk_size_t) population < num_populations
            && is_source[population]) {
            return population;
        }
        node = tree->parent[node];
    }
    return TSK_NULL;
}

SEXP
RC_rtskit_ancestry_intervals(
    SEXP ptr, SEXP sample_ids, SEXP source_ids, SEXP require_resolved_value)
{
    rtskit_treeseq_handle *handle = rtskit_get_handle(ptr);
    const tsk_treeseq_t *ts = &handle->value;
    const tsk_node_table_t *nodes = &ts->tables->nodes;
    tsk_size_t num_nodes = nodes->num_rows;
    tsk_size_t num_populations = ts->tables->populations.num_rows;
    R_xlen_t num_samples;
    R_xlen_t num_sources;
    int require_resolved;
    tsk_tree_t tree;
    int tree_initialized = 0;
    int ret;
    tsk_id_t unresolved_sample = TSK_NULL;
    tsk_id_t unresolved_tree = TSK_NULL;

    if (TYPEOF(sample_ids) != INTSXP || TYPEOF(source_ids) != INTSXP) {
        Rf_error("samples and source_populations must be integer vectors");
    }
    if (TYPEOF(require_resolved_value) != LGLSXP
        || XLENGTH(require_resolved_value) != 1
        || LOGICAL(require_resolved_value)[0] == NA_LOGICAL) {
        Rf_error("require_resolved must be TRUE or FALSE");
    }
    require_resolved = LOGICAL(require_resolved_value)[0];
    num_samples = XLENGTH(sample_ids);
    num_sources = XLENGTH(source_ids);
    if (num_sources == 0) {
        Rf_error("source_populations must contain at least one population ID");
    }
    if (num_samples > INT_MAX) {
        Rf_error("too many focal samples for the R data-frame interface");
    }

    unsigned char *is_source
        = (unsigned char *) R_alloc(num_populations > 0 ? num_populations : 1, 1);
    memset(is_source, 0, num_populations > 0 ? num_populations : 1);
    for (R_xlen_t j = 0; j < num_sources; j++) {
        int source = INTEGER(source_ids)[j];
        if (source < 0 || (tsk_size_t) source >= num_populations) {
            Rf_error("source population ID is out of range");
        }
        is_source[source] = 1;
    }
    for (R_xlen_t j = 0; j < num_samples; j++) {
        int sample = INTEGER(sample_ids)[j];
        if (sample < 0 || (tsk_size_t) sample >= num_nodes) {
            Rf_error("sample node ID is out of range");
        }
        if (!(nodes->flags[sample] & TSK_NODE_IS_SAMPLE)) {
            Rf_error("focal node %d is not marked as a sample", sample);
        }
    }

    tsk_size_t *counts = (tsk_size_t *) R_alloc(num_samples > 0 ? num_samples : 1,
        sizeof(*counts));
    tsk_id_t *current = (tsk_id_t *) R_alloc(num_samples > 0 ? num_samples : 1,
        sizeof(*current));
    memset(counts, 0, (num_samples > 0 ? num_samples : 1) * sizeof(*counts));
    for (R_xlen_t j = 0; j < num_samples; j++) current[j] = INT_MIN;

    ret = tsk_tree_init(&tree, ts, 0);
    if (ret < 0) rtskit_error(ret, "could not initialize ancestry tree iterator");
    tree_initialized = 1;
    for (ret = tsk_tree_first(&tree); ret == TSK_TREE_OK;
         ret = tsk_tree_next(&tree)) {
        for (R_xlen_t j = 0; j < num_samples; j++) {
            tsk_id_t sample = INTEGER(sample_ids)[j];
            tsk_id_t source = rtskit_find_source_population(
                &tree, nodes, sample, is_source, num_populations);
            if (source == TSK_NULL && require_resolved) {
                unresolved_sample = sample;
                unresolved_tree = tree.index;
                goto ancestry_unresolved;
            }
            if (current[j] == INT_MIN || source != current[j]) {
                if (counts[j] == (tsk_size_t) R_XLEN_T_MAX) {
                    tsk_tree_free(&tree);
                    Rf_error("too many ancestry intervals for R");
                }
                counts[j]++;
                current[j] = source;
            }
        }
    }
    if (ret < 0) {
        tsk_tree_free(&tree);
        rtskit_error(ret, "ancestry tree iteration failed");
    }
    tsk_tree_free(&tree);
    tree_initialized = 0;

    tsk_size_t total = 0;
    tsk_size_t *offset = (tsk_size_t *) R_alloc(num_samples + 1, sizeof(*offset));
    tsk_size_t *written = (tsk_size_t *) R_alloc(num_samples > 0 ? num_samples : 1,
        sizeof(*written));
    offset[0] = 0;
    for (R_xlen_t j = 0; j < num_samples; j++) {
        if (counts[j] > (tsk_size_t) INT_MAX - total) {
            Rf_error("too many ancestry intervals for the R data-frame interface");
        }
        total += counts[j];
        offset[j + 1] = total;
        current[j] = INT_MIN;
        written[j] = 0;
    }

    const char *names[] = { "sample", "left", "right", "source_population" };
    SEXP result = PROTECT(Rf_allocVector(VECSXP, 4));
    SEXP sample_column = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t) total));
    SEXP left_column = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t) total));
    SEXP right_column = PROTECT(Rf_allocVector(REALSXP, (R_xlen_t) total));
    SEXP source_column = PROTECT(Rf_allocVector(INTSXP, (R_xlen_t) total));
    double *segment_left
        = (double *) R_alloc(num_samples > 0 ? num_samples : 1, sizeof(*segment_left));

    ret = tsk_tree_init(&tree, ts, 0);
    if (ret < 0) {
        UNPROTECT(5);
        rtskit_error(ret, "could not initialize ancestry tree iterator");
    }
    tree_initialized = 1;
    for (ret = tsk_tree_first(&tree); ret == TSK_TREE_OK;
         ret = tsk_tree_next(&tree)) {
        for (R_xlen_t j = 0; j < num_samples; j++) {
            tsk_id_t sample = INTEGER(sample_ids)[j];
            tsk_id_t source = rtskit_find_source_population(
                &tree, nodes, sample, is_source, num_populations);
            if (current[j] == INT_MIN) {
                current[j] = source;
                segment_left[j] = tree.interval.left;
            } else if (source != current[j]) {
                tsk_size_t row = offset[j] + written[j]++;
                INTEGER(sample_column)[row] = sample;
                REAL(left_column)[row] = segment_left[j];
                REAL(right_column)[row] = tree.interval.left;
                INTEGER(source_column)[row] = current[j];
                current[j] = source;
                segment_left[j] = tree.interval.left;
            }
        }
    }
    if (ret < 0) {
        tsk_tree_free(&tree);
        UNPROTECT(5);
        rtskit_error(ret, "ancestry tree iteration failed");
    }
    for (R_xlen_t j = 0; j < num_samples; j++) {
        if (current[j] != INT_MIN) {
            tsk_size_t row = offset[j] + written[j]++;
            INTEGER(sample_column)[row] = INTEGER(sample_ids)[j];
            REAL(left_column)[row] = segment_left[j];
            REAL(right_column)[row] = tsk_treeseq_get_sequence_length(ts);
            INTEGER(source_column)[row] = current[j];
        }
        if (written[j] != counts[j]) {
            tsk_tree_free(&tree);
            UNPROTECT(5);
            Rf_error("ancestry interval count changed between native traversals");
        }
    }
    tsk_tree_free(&tree);
    tree_initialized = 0;

    SET_VECTOR_ELT(result, 0, sample_column);
    SET_VECTOR_ELT(result, 1, left_column);
    SET_VECTOR_ELT(result, 2, right_column);
    SET_VECTOR_ELT(result, 3, source_column);
    rtskit_set_names(result, names, 4);
    rtskit_set_data_frame(result, (int) total);
    UNPROTECT(5);
    return result;

ancestry_unresolved:
    if (tree_initialized) tsk_tree_free(&tree);
    Rf_error("sample node %d has no declared source-population ancestor in tree %d",
        unresolved_sample, unresolved_tree);
    return R_NilValue;
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
    const char *json_schema = "{\"codec\":\"json\",\"type\":\"object\"}";
    const char *source_a = "{\"name\":\"source_A\"}";
    const char *source_b = "{\"name\":\"source_B\"}";
    const char *target = "{\"name\":\"target\"}";
    double location[] = { 1.5, 2.5 };

    ret = tsk_population_table_set_metadata_schema(
        &tables.populations, json_schema, strlen(json_schema));
    if (ret < 0) goto example_error;
    ret = tsk_individual_table_set_metadata_schema(
        &tables.individuals, json_schema, strlen(json_schema));
    if (ret < 0) goto example_error;
    ret = tsk_population_table_add_row(
        &tables.populations, source_a, strlen(source_a));
    if (ret < 0) goto example_error;
    ret = tsk_population_table_add_row(
        &tables.populations, source_b, strlen(source_b));
    if (ret < 0) goto example_error;
    ret = tsk_individual_table_add_row(&tables.individuals, 0, location, 2,
        NULL, 0, target, strlen(target));
    if (ret < 0) goto example_error;

    ret = tsk_node_table_add_row(
        &tables.nodes, TSK_NODE_IS_SAMPLE, 0.0, TSK_NULL, 0, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_node_table_add_row(
        &tables.nodes, TSK_NODE_IS_SAMPLE, 0.0, TSK_NULL, 0, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_node_table_add_row(&tables.nodes, 0, 1.0, 0, TSK_NULL, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_node_table_add_row(&tables.nodes, 0, 1.0, 1, TSK_NULL, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_edge_table_add_row(&tables.edges, 0.0, 5.0, 2, 0, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_edge_table_add_row(&tables.edges, 0.0, 5.0, 2, 1, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_edge_table_add_row(&tables.edges, 5.0, 10.0, 3, 0, NULL, 0);
    if (ret < 0) goto example_error;
    ret = tsk_edge_table_add_row(&tables.edges, 5.0, 10.0, 3, 1, NULL, 0);
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
