.rtskit_scalar_string <- S7::new_property(
  class = S7::class_character,
  validator = function(value) {
    if (length(value) != 1L || is.na(value) || !nzchar(value)) {
      "must be one non-missing, non-empty string"
    }
  }
)

.rtskit_pointer <- S7::new_property(
  class = S7::class_any,
  validator = function(value) {
    valid <- typeof(value) == "externalptr" &&
      isTRUE(tryCatch(
        .Call(C_RC_rtskit_is_valid, value),
        error = function(condition) FALSE
      ))
    if (!valid) "must be a live Rtskit tree-sequence pointer"
  }
)

.rtskit_assert_tree_sequence <- function(value) {
  if (!isTRUE(tryCatch(
    S7::S7_inherits(value, TreeSequence),
    error = function(condition) FALSE
  ))) {
    stop("x must be a TreeSequence", call. = FALSE)
  }
  if (!isTRUE(.Call(C_RC_rtskit_is_valid, value@ptr))) {
    stop("x no longer owns a live native tree sequence", call. = FALSE)
  }
  value
}

.rtskit_assert_ids <- function(value, argument, allow_empty = TRUE) {
  if (!is.numeric(value) || anyNA(value) || any(!is.finite(value)) ||
      any(value != trunc(value)) || any(value < 0) ||
      any(value > .Machine$integer.max)) {
    stop(argument, " must contain non-negative integer IDs", call. = FALSE)
  }
  if (!allow_empty && length(value) == 0L) {
    stop(argument, " must not be empty", call. = FALSE)
  }
  as.integer(value)
}

.rtskit_assert_flag <- function(value, argument) {
  if (!is.logical(value) || length(value) != 1L || is.na(value)) {
    stop(argument, " must be TRUE or FALSE", call. = FALSE)
  }
  value
}

.rtskit_assert_path <- function(path, argument, must_work) {
  if (!is.character(path) || length(path) != 1L || is.na(path) || !nzchar(path)) {
    stop(argument, " must be one non-missing, non-empty path", call. = FALSE)
  }
  normalizePath(path, winslash = "/", mustWork = must_work)
}
