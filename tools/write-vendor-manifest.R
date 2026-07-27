args <- commandArgs(FALSE)
script <- sub("^--file=", "", args[grep("^--file=", args)])
root <- normalizePath(file.path(dirname(script), ".."), mustWork = TRUE)
vendor <- file.path(root, "src", "vendor")
files <- list.files(vendor, recursive = TRUE, full.names = TRUE)
files <- files[!grepl("(^|/)manifest\\.tsv$", files)]
files <- files[file.info(files)$isdir %in% FALSE & !grepl("\\.o$", files)]
relative <- substring(files, nchar(root) + 2L)
order <- order(relative)
relative <- relative[order]
files <- files[order]
out <- data.frame(
  path = relative,
  md5 = unname(tools::md5sum(files)),
  stringsAsFactors = FALSE
)
write.table(
  out,
  file.path(vendor, "manifest.tsv"),
  sep = "\t",
  row.names = FALSE,
  quote = FALSE
)
