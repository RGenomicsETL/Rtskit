args <- commandArgs(trailingOnly = TRUE)
if (length(args) != 1L) stop("usage: check-upstream-vendor.R ROOT", call. = FALSE)
root <- normalizePath(args[[1L]], mustWork = TRUE)
repo <- normalizePath(file.path(dirname(sub(
  "^--file=", "", commandArgs(FALSE)[grep("^--file=", commandArgs(FALSE))]
)), ".."), mustWork = TRUE)
manifest <- read.delim(
  file.path(repo, "tools", "upstream-manifest.tsv"),
  stringsAsFactors = FALSE,
  check.names = FALSE
)
files <- file.path(root, manifest$path)
if (!all(file.exists(files))) {
  stop("reconstructed upstream inventory is incomplete", call. = FALSE)
}
actual <- unname(tools::md5sum(files))
if (!identical(actual, manifest$md5)) {
  bad <- manifest$path[actual != manifest$md5]
  stop("reconstructed upstream mismatch: ", paste(bad, collapse = ", "), call. = FALSE)
}
actual_files <- list.files(file.path(root, "src", "vendor"), recursive = TRUE, full.names = TRUE)
actual_rel <- sort(substring(actual_files, nchar(root) + 2L))
if (!identical(actual_rel, sort(manifest$path))) {
  stop("reconstructed upstream inventory has undeclared files", call. = FALSE)
}
cat("Vendor patch reconstructs the pinned upstream source exactly.\n")
