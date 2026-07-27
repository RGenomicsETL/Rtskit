x <- tskit_example()
expect_true(S7::S7_inherits(x, TreeSequence))

info <- tskit_summary(x)
expect_identical(info$sequence_length, 10)
expect_identical(info$trees, 2)
expect_identical(info$samples, 2)
expect_identical(info$individuals, 1)
expect_identical(info$nodes, 4)
expect_identical(info$edges, 4)
expect_identical(info$sites, 1)
expect_identical(info$mutations, 1)
expect_identical(info$populations, 2)
expect_identical(tskit_samples(x), 0:1)

nodes <- tskit_nodes(x)
expect_identical(nodes$id, 0:3)
expect_identical(nodes$time, c(0, 0, 1, 1))
expect_identical(nodes$population, c(-1L, -1L, 0L, 1L))
expect_identical(nodes$individual, c(0L, 0L, -1L, -1L))

edges <- tskit_edges(x)
expect_identical(edges$id, 0:3)
expect_identical(edges$left, c(0, 0, 5, 5))
expect_identical(edges$right, c(5, 5, 10, 10))
expect_identical(edges$parent, c(2L, 2L, 3L, 3L))
expect_identical(edges$child, c(0L, 1L, 0L, 1L))

trees <- tskit_trees(x)
expect_identical(trees$index, 0:1)
expect_identical(trees$left, c(0, 5))
expect_identical(trees$right, c(5, 10))
expect_identical(trees$roots, c(1, 1))
expect_identical(trees$edges, c(2, 2))

populations <- tskit_populations(x)
expect_identical(populations$id, 0:1)
expect_identical(populations$metadata_length, c(19, 19))
individuals <- tskit_individuals(x)
expect_identical(individuals$id, 0L)
expect_identical(individuals$location[[1L]], c(1.5, 2.5))
expect_identical(individuals$parents[[1L]], integer())
expect_identical(individuals$metadata_length, 17)

population_metadata <- tskit_metadata(x, "populations")
expect_identical(
  lapply(population_metadata, rawToChar),
  list('{"name":"source_A"}', '{"name":"source_B"}')
)
expect_identical(
  rawToChar(tskit_metadata(x, "populations", 1L)[[1L]]),
  '{"name":"source_B"}'
)
expect_identical(
  rawToChar(tskit_metadata(x, "individuals", 0L)[[1L]]),
  '{"name":"target"}'
)
expect_identical(
  rawToChar(tskit_metadata_schema(x, "populations")),
  '{"codec":"json","type":"object"}'
)
expect_identical(tskit_metadata(x, "tree_sequence"), raw())
expect_error(tskit_metadata(x, "tree_sequence", 0L), "top-level")
expect_error(tskit_metadata(x, "populations", 2L), "out of range")

ancestry <- tskit_ancestry_intervals(
  x,
  c("panel:source_A" = 0L, "panel:source_B" = 1L)
)
expect_identical(ancestry$sample, c(0L, 0L, 1L, 1L))
expect_identical(ancestry$left, c(0, 5, 0, 5))
expect_identical(ancestry$right, c(5, 10, 5, 10))
expect_identical(
  ancestry$source,
  c("panel:source_A", "panel:source_B", "panel:source_A", "panel:source_B")
)
expect_identical(ancestry$source_population, c(0L, 1L, 0L, 1L))
expect_identical(
  tskit_ancestry_intervals(x, 0L, samples = integer()),
  data.frame(
    sample = integer(), left = numeric(), right = numeric(),
    source = character(), source_population = integer()
  )
)
expect_error(tskit_ancestry_intervals(x, 0L), "no declared source")
unresolved <- tskit_ancestry_intervals(x, c("panel:source_A" = 0L),
  samples = 0L, require_resolved = FALSE)
expect_identical(unresolved$source_population, c(0L, -1L))
expect_identical(unresolved$source, c("panel:source_A", NA_character_))
expect_error(tskit_ancestry_intervals(x, 2L), "out of range")
expect_error(tskit_ancestry_intervals(x, 0L, samples = 2L), "not marked")
expect_error(tskit_ancestry_intervals(x, c(a = 0L, a = 1L)), "names")

path <- tempfile(fileext = ".trees")
expect_identical(tskit_dump(x, path), normalizePath(path, winslash = "/"))
expect_error(tskit_dump(x, path), "already exists")
y <- tskit_load(path)
expect_identical(tskit_summary(y), info)
expect_identical(tskit_nodes(y), nodes)
expect_identical(tskit_edges(y), edges)
expect_identical(tskit_populations(y), populations)
expect_identical(tskit_individuals(y), individuals)
expect_identical(tskit_metadata(y, "populations"), population_metadata)
expect_identical(
  tskit_ancestry_intervals(y, c(A = 0L, B = 1L))$source_population,
  ancestry$source_population
)
unlink(path)

expect_error(tskit_load(tempfile()), "path")
expect_error(tskit_nodes(list()), "TreeSequence")
