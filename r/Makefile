# MAKEFILE
#
# @author      Nicola Asuni <info@tecnick.com>
# @link        https://github.com/tecnickcom/variantkey
# ------------------------------------------------------------------------------

# To install R please refere to the official guides at: https://cran.r-project.org
#
# For Ubuntu:
#
#     cd /tmp
#     wget -qO- https://cloud.r-project.org/bin/linux/ubuntu/marutter_pubkey.asc | sudo gpg --dearmor -o /usr/share/keyrings/r-project.gpg
#     echo "deb [signed-by=/usr/share/keyrings/r-project.gpg] https://cloud.r-project.org/bin/linux/ubuntu jammy-cran40/" | sudo tee -a /etc/apt/sources.list.d/r-project.list
#     sudo apt update
#     sudo apt install --no-install-recommends r-base
#
# Install additional R packages required for this project:
#     sudo Rscript -e "install.packages(c('Rcpp', 'devtools', 'inline', 'renv', 'ragg', 'roxygen2', 'testthat', 'pkgdown'), repos = 'http://cran.us.r-project.org')"
#
# ------------------------------------------------------------------------------

SHELL=/bin/bash
.SHELLFLAGS=-o pipefail -c

# Project name
PROJECT=variantkey

# Project version
VERSION=$(shell cat ../VERSION)

# Project release number (packaging build number)
RELEASE=$(shell cat ../RELEASE)

# Current directory
CURRENTDIR=$(CURDIR)/

# sed argument for in-place substitutions
SEDINPLACE=-i
ifeq ($(shell uname -s),Darwin)
	SEDINPLACE=-i ''
endif

# --- MAKE TARGETS ---

.PHONY: help
help:
	@echo ""
	@echo "$(PROJECT) Makefile."
	@echo "The following commands are available:"
	@echo ""
	@awk '/^## /{desc=substr($$0,4)} /^\.PHONY:/{if(NF>1) {target=$$2; if(desc) printf "  make %-15s: %s\n",target,desc; desc=""}}' Makefile
	@echo ""

all: clean format build test

## Set the version from VERSION file
.PHONY: version
version:
	sed $(SEDINPLACE) "s/Version:.*$$/Version: $(VERSION).$(RELEASE)/" variantkey/DESCRIPTION

## Build the library
.PHONY: build
build: version
	cd variantkey \
	&& cp ../../LICENSE .
	R -e 'setwd("./variantkey");renv::restore(prompt=FALSE);pkgbuild::build();'

## Re-build lockfile dependencies
.PHONY: rebuild
rebuild:
	R -e 'setwd("./variantkey");lock<-renv::lockfile_read("renv.lock");renv::rebuild(packages=names(lock$$Packages))'

## Unit tests
.PHONY: test
test:
	R -e 'setwd("./variantkey");renv::restore(prompt=FALSE);renv::install(".",prompt=FALSE);testthat::test_dir("tests")'

## Generate docs and run unit tests
.PHONY: testdoc
testdoc:
	R -e 'setwd("./variantkey");renv::restore(prompt=FALSE);renv::install(c("cpp11","vctrs","roxygen2"),prompt=FALSE);roxygen2::roxygenise();testthat::test_dir("tests")'

## Format the source code
.PHONY: format
format:
	astyle --style=allman --recursive --suffix=none 'variantkey/src/*.c'
	astyle --style=allman --recursive --suffix=none 'variantkey/src/*.h'

## Remove any build artifact
.PHONY: clean
clean:
	rm -f variantkey.Rcheck variantkey/src/variantkey.o variantkey/src/variantkey.so
	rm -rf renv variantkey/renv variantkey/.Rbuildignore variantkey/.Rprofile .Rprofile

## Generate documentation
.PHONY: devdoc
devdoc:
	R -e 'setwd("./variantkey");renv::restore(prompt=FALSE);renv::install(c("cpp11","vctrs","roxygen2"),prompt=FALSE);roxygen2::roxygenise()'
