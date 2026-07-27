PKGNAME := Rtskit
PKGVERS := $(shell sed -n 's/Version: *\([^ ]*\)/\1/p' DESCRIPTION)
TARBALL := $(PKGNAME)_$(PKGVERS).tar.gz

all: check

rd:
	Rscript -e 'roxygen2::roxygenize()'

vendor-audit:
	Rscript tools/check-vendor.R
	patches/check.sh

install: rd vendor-audit
	R CMD INSTALL --preclean .

test: install
	Rscript -e 'tinytest::test_package("$(PKGNAME)")'

build: rd vendor-audit
	R CMD build .

check: build
	R CMD check --no-manual $(TARBALL)

clean:
	rm -rf $(TARBALL) $(PKGNAME).Rcheck
	rm -f src/*.o src/*.so src/*.dll src/vendor/tskit/tskit/*.o src/vendor/kastore/*.o

.PHONY: all rd vendor-audit install test build check clean
