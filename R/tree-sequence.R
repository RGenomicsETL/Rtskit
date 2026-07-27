#' A native succinct tree sequence
#'
#' `TreeSequence` owns one `tsk_treeseq_t` value through a native external
#' pointer. IDs returned by table accessors retain tskit's zero-based indexing.
#'
#' @param ptr Internal live native pointer.
#' @param source Source path or `"<memory>"`.
#' @export
TreeSequence <- S7::new_class(
  "TreeSequence",
  package = "Rtskit",
  properties = list(
    ptr = .rtskit_pointer,
    source = .rtskit_scalar_string
  )
)

#' Load a succinct tree sequence
#'
#' @param path Existing `.trees` file.
#' @return A `TreeSequence` owning the loaded native value.
#' @export
tskit_load <- function(path) {
  path <- .rtskit_assert_path(path, "path", must_work = TRUE)
  if (dir.exists(path)) stop("path must identify a file", call. = FALSE)
  TreeSequence(ptr = .Call(C_RC_rtskit_load, path), source = path)
}

#' Create deterministic example tree-sequence data
#'
#' The example contains two sample nodes, one ancestor, one tree over a
#' sequence of length 10, one site, and one mutation. It exercises the native
#' constructor without external software.
#'
#' @return An in-memory `TreeSequence`.
#' @export
tskit_example <- function() {
  TreeSequence(ptr = .Call(C_RC_rtskit_example), source = "<memory>")
}

#' Write a succinct tree sequence
#'
#' @param x A `TreeSequence`.
#' @param path Destination `.trees` path. Its parent directory must exist.
#' @param overwrite Whether to replace an existing file.
#' @return The normalized destination path, invisibly.
#' @export
tskit_dump <- function(x, path, overwrite = FALSE) {
  x <- .rtskit_assert_tree_sequence(x)
  if (!is.logical(overwrite) || length(overwrite) != 1L || is.na(overwrite)) {
    stop("overwrite must be TRUE or FALSE", call. = FALSE)
  }
  if (!is.character(path) || length(path) != 1L || is.na(path) || !nzchar(path)) {
    stop("path must be one non-missing, non-empty path", call. = FALSE)
  }
  path <- path.expand(path)
  parent <- normalizePath(dirname(path), winslash = "/", mustWork = TRUE)
  path <- file.path(parent, basename(path))
  if (dir.exists(path)) stop("path identifies a directory", call. = FALSE)
  if (file.exists(path) && !overwrite) {
    stop("path already exists; set overwrite = TRUE to replace it", call. = FALSE)
  }
  if (file.exists(path)) unlink(path)
  invisible(.Call(C_RC_rtskit_dump, x@ptr, path))
}

#' Summarize a succinct tree sequence
#'
#' @param x A `TreeSequence`.
#' @return A named list of sequence length and table/tree counts. Counts are
#'   doubles because tskit sizes can exceed R's integer range.
#' @export
tskit_summary <- function(x) {
  x <- .rtskit_assert_tree_sequence(x)
  .Call(C_RC_rtskit_summary, x@ptr)
}

#' Return sample node IDs
#'
#' @inheritParams tskit_summary
#' @return An integer vector of zero-based node IDs.
#' @export
tskit_samples <- function(x) {
  x <- .rtskit_assert_tree_sequence(x)
  .Call(C_RC_rtskit_samples, x@ptr)
}

#' Materialize the node table
#'
#' Metadata remains native and is not decoded by this initial accessor.
#'
#' @inheritParams tskit_summary
#' @return A data frame with zero-based IDs, flags, time, population, and
#'   individual columns.
#' @export
tskit_nodes <- function(x) {
  x <- .rtskit_assert_tree_sequence(x)
  .Call(C_RC_rtskit_nodes, x@ptr)
}

#' Materialize the edge table
#'
#' Metadata remains native and is not decoded by this initial accessor.
#'
#' @inheritParams tskit_summary
#' @return A data frame with zero-based IDs and edge coordinates and nodes.
#' @export
tskit_edges <- function(x) {
  x <- .rtskit_assert_tree_sequence(x)
  .Call(C_RC_rtskit_edges, x@ptr)
}

#' Traverse tree intervals
#'
#' @inheritParams tskit_summary
#' @return A data frame containing each tree index, half-open interval, root
#'   count, and active edge count.
#' @export
tskit_trees <- function(x) {
  x <- .rtskit_assert_tree_sequence(x)
  .Call(C_RC_rtskit_trees, x@ptr)
}

S7::method(print, TreeSequence) <- function(x, ...) {
  values <- tskit_summary(x)
  cat(
    "<Rtskit::TreeSequence>",
    "\n  source: ", x@source,
    "\n  sequence length: ", format(values$sequence_length, scientific = FALSE),
    "\n  trees: ", format(values$trees, scientific = FALSE),
    "\n  samples: ", format(values$samples, scientific = FALSE),
    "\n",
    sep = ""
  )
  invisible(x)
}

S7::method(summary, TreeSequence) <- function(object, ...) {
  tskit_summary(object)
}
