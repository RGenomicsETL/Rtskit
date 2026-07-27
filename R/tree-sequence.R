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
#' The example contains two sample nodes from one individual and two source
#' ancestors spanning adjacent trees over a sequence of length 10. Population
#' and individual metadata, one site, and one mutation exercise the native
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

#' Materialize the population table
#'
#' Population metadata remains opaque. Use [tskit_metadata()] to request its
#' bytes explicitly.
#'
#' @inheritParams tskit_summary
#' @return A data frame with zero-based population IDs and metadata lengths.
#' @export
tskit_populations <- function(x) {
  x <- .rtskit_assert_tree_sequence(x)
  .Call(C_RC_rtskit_populations, x@ptr)
}

#' Materialize the individual table
#'
#' Ragged locations and parent IDs are returned as list columns. Individual
#' metadata remains opaque and is available through [tskit_metadata()].
#'
#' @inheritParams tskit_summary
#' @return A data frame with zero-based individual IDs, flags, location and
#'   parent list columns, and metadata lengths.
#' @export
tskit_individuals <- function(x) {
  x <- .rtskit_assert_tree_sequence(x)
  .Call(C_RC_rtskit_individuals, x@ptr)
}

#' Read opaque metadata bytes
#'
#' Rtskit deliberately returns raw bytes and does not infer a codec from a
#' schema. Row tables return a list of raw vectors in the requested ID order.
#' Top-level tree-sequence and reference-sequence metadata return one raw
#' vector and do not accept `id`.
#'
#' @inheritParams tskit_summary
#' @param table One of `"tree_sequence"`, `"reference_sequence"`,
#'   `"individuals"`, `"nodes"`, `"edges"`, `"migrations"`, `"sites"`,
#'   `"mutations"`, or `"populations"`.
#' @param id Optional zero-based row IDs. `NULL` requests every row of a row
#'   table.
#' @return A raw vector for top-level metadata or a list of raw vectors for a
#'   row table.
#' @export
tskit_metadata <- function(x, table, id = NULL) {
  x <- .rtskit_assert_tree_sequence(x)
  tables <- c(
    "tree_sequence", "reference_sequence", "individuals", "nodes", "edges",
    "migrations", "sites", "mutations", "populations"
  )
  table <- match.arg(table, tables)
  top_level <- table %in% c("tree_sequence", "reference_sequence")
  if (top_level) {
    if (!is.null(id)) stop("id is not defined for top-level metadata", call. = FALSE)
    id <- integer()
  } else if (is.null(id)) {
    rows <- tskit_summary(x)[[table]]
    if (rows > .Machine$integer.max) {
      stop("table has too many rows for integer IDs", call. = FALSE)
    }
    id <- seq_len(as.integer(rows)) - 1L
  } else {
    id <- .rtskit_assert_ids(id, "id")
  }
  .Call(C_RC_rtskit_metadata, x@ptr, table, id)
}

#' Read an opaque metadata schema
#'
#' The schema is returned exactly as stored. Convert or decode it explicitly
#' only when the declared codec is understood by the caller.
#'
#' @inheritParams tskit_metadata
#' @return A raw vector containing the table metadata schema.
#' @export
tskit_metadata_schema <- function(x, table) {
  x <- .rtskit_assert_tree_sequence(x)
  table <- match.arg(table, c(
    "tree_sequence", "reference_sequence", "individuals", "nodes", "edges",
    "migrations", "sites", "mutations", "populations"
  ))
  .Call(C_RC_rtskit_metadata_schema, x@ptr, table)
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

#' Extract source-population ancestry intervals
#'
#' For every focal sample and marginal tree, this function walks toward the
#' root and records the first node whose population is one of
#' `source_populations`. Adjacent tree intervals with the same source are
#' merged. This is exact ancestry truth only when the simulation's population
#' and node-retention semantics make those source populations authoritative.
#'
#' @inheritParams tskit_summary
#' @param source_populations A non-empty vector of zero-based source-population
#'   IDs. Names, when supplied, become the returned source labels and should be
#'   dataset-qualified.
#' @param samples Zero-based focal sample-node IDs. `NULL` uses every sample.
#' @param require_resolved Whether to fail if a focal lineage reaches a root
#'   without encountering a declared source population.
#' @return A data frame of sample IDs, half-open intervals, source labels, and
#'   source-population IDs, grouped by sample and ordered by coordinate.
#' @references Haller et al. (2019), *Tree-sequence recording in SLiM opens new
#'   horizons for forward-time simulation of whole genomes*,
#'   \doi{10.1111/1755-0998.12968}. The published local-ancestry example is
#'   pinned at SLiMTreeSeqPub commit
#'   `6715c28b02942bc4757c9f8bcab133ad4a0bfcfb`.
#' @export
tskit_ancestry_intervals <- function(
  x,
  source_populations,
  samples = NULL,
  require_resolved = TRUE
) {
  x <- .rtskit_assert_tree_sequence(x)
  labels <- names(source_populations)
  source_populations <- .rtskit_assert_ids(
    source_populations, "source_populations", allow_empty = FALSE
  )
  if (anyDuplicated(source_populations)) {
    stop("source_populations must contain unique IDs", call. = FALSE)
  }
  if (is.null(labels)) {
    labels <- as.character(source_populations)
  } else if (anyNA(labels) || any(!nzchar(labels)) || anyDuplicated(labels)) {
    stop("source_populations names must be non-empty and unique", call. = FALSE)
  }
  if (is.null(samples)) samples <- tskit_samples(x)
  samples <- .rtskit_assert_ids(samples, "samples")
  if (anyDuplicated(samples)) stop("samples must contain unique IDs", call. = FALSE)
  require_resolved <- .rtskit_assert_flag(require_resolved, "require_resolved")

  result <- .Call(
    C_RC_rtskit_ancestry_intervals,
    x@ptr,
    samples,
    source_populations,
    require_resolved
  )
  matched <- match(result$source_population, source_populations)
  result$source <- labels[matched]
  result[c("sample", "left", "right", "source", "source_population")]
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
