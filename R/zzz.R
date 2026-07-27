#' Rtskit: native access to succinct tree sequences
#'
#' Rtskit directly wraps the bundled, pinned tskit C API. It has no Python,
#' Rust, Java, or external shared-library runtime dependency.
#'
#' @useDynLib Rtskit, .registration = TRUE, .fixes = "C_"
#' @keywords internal
#' @name Rtskit-package
"_PACKAGE"

.onLoad <- function(...) {
  S7::methods_register()
  invisible(.Call(C_RC_rtskit_initialize, "Rtskit::TreeSequence"))
}
