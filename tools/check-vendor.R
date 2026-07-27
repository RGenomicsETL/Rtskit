args <- commandArgs(FALSE)
script <- sub("^--file=", "", args[grep("^--file=", args)])
root <- normalizePath(file.path(dirname(script), ".."), mustWork = TRUE)
vendor <- file.path(root, "src", "vendor")
manifest_path <- file.path(vendor, "manifest.tsv")
if (!file.exists(manifest_path)) stop("vendor manifest is missing", call. = FALSE)
manifest <- read.delim(manifest_path, stringsAsFactors = FALSE, check.names = FALSE)
expected_columns <- c("path", "md5")
if (!identical(names(manifest), expected_columns)) {
  stop("vendor manifest columns are invalid", call. = FALSE)
}
files <- list.files(vendor, recursive = TRUE, full.names = TRUE)
files <- files[
  basename(files) != "manifest.tsv" &
    file.info(files)$isdir %in% FALSE &
    !grepl("\\.o$", files)
]
relative <- sort(substring(files, nchar(root) + 2L))
if (!identical(relative, sort(manifest$path))) {
  stop("vendor file inventory differs from manifest", call. = FALSE)
}
actual <- unname(tools::md5sum(file.path(root, manifest$path)))
if (!identical(actual, manifest$md5)) {
  bad <- manifest$path[actual != manifest$md5]
  stop("vendor checksum mismatch: ", paste(bad, collapse = ", "), call. = FALSE)
}
version <- readLines(file.path(vendor, "tskit", "VERSION.txt"), warn = FALSE)
kastore <- readLines(file.path(vendor, "kastore", "VERSION.txt"), warn = FALSE)
if (!identical(version, "1.3.1") || !identical(kastore, "2.1.2")) {
  stop("vendored native versions differ from tools/upstream.json", call. = FALSE)
}
cat("Vendored tskit C 1.3.1 and kastore 2.1.2 match the recorded manifest.\n")
