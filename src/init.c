#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>
#include <Rinternals.h>

extern SEXP RC_rtskit_initialize(SEXP);
extern SEXP RC_rtskit_is_valid(SEXP);
extern SEXP RC_rtskit_load(SEXP);
extern SEXP RC_rtskit_dump(SEXP, SEXP);
extern SEXP RC_rtskit_summary(SEXP);
extern SEXP RC_rtskit_samples(SEXP);
extern SEXP RC_rtskit_nodes(SEXP);
extern SEXP RC_rtskit_edges(SEXP);
extern SEXP RC_rtskit_trees(SEXP);
extern SEXP RC_rtskit_example(void);

static const R_CallMethodDef call_methods[] = {
    { "RC_rtskit_initialize", (DL_FUNC) &RC_rtskit_initialize, 1 },
    { "RC_rtskit_is_valid", (DL_FUNC) &RC_rtskit_is_valid, 1 },
    { "RC_rtskit_load", (DL_FUNC) &RC_rtskit_load, 1 },
    { "RC_rtskit_dump", (DL_FUNC) &RC_rtskit_dump, 2 },
    { "RC_rtskit_summary", (DL_FUNC) &RC_rtskit_summary, 1 },
    { "RC_rtskit_samples", (DL_FUNC) &RC_rtskit_samples, 1 },
    { "RC_rtskit_nodes", (DL_FUNC) &RC_rtskit_nodes, 1 },
    { "RC_rtskit_edges", (DL_FUNC) &RC_rtskit_edges, 1 },
    { "RC_rtskit_trees", (DL_FUNC) &RC_rtskit_trees, 1 },
    { "RC_rtskit_example", (DL_FUNC) &RC_rtskit_example, 0 },
    { NULL, NULL, 0 }
};

void attribute_visible
R_init_Rtskit(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, call_methods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
