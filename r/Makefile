# MAKEFILE
#
# @author      Nicola Asuni <info@tecnick.com>
# @link        https://github.com/tecnickcom/variantkey
# ------------------------------------------------------------------------------

# Install R in Ubuntu/Debian:
# sudo apt install r-base libfontconfig1-dev libcurl4-openssl-dev libharfbuzz-dev libfribidi-dev libfreetype6-dev libpng-dev libtiff5-dev libjpeg-dev
# sudo Rscript -e "install.packages(c('Rcpp', 'base', 'devtools', 'inline', 'pryr', 'renv', 'ragg', 'roxygen2', 'testthat', 'pkgdown', 'libgfortran-ng'), repos = 'http://cran.us.r-project.org')"

# Project name
PROJECT=variantkey

# Project version
VERSION=$(shell cat ../VERSION)

# Project release number (packaging build number)
RELEASE=$(shell cat ../RELEASE)

# Current directory
CURRENTDIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))

# --- MAKE TARGETS ---

# Display general help about this command
.PHONY: help
help:
	@echo ""
	@echo "VariantKey R Makefile."
	@echo "The following commands are available:"
	@echo ""
	@echo "    make build   : Build the library"
	@echo "    make test    : Test the library"
	@echo "    make format  : Format the source code"
	@echo "    make clean   : Remove any build artifact"
	@echo "    make devdoc  : Generate documentation"
	@echo ""

all: clean format build test

# Set the version from VERSION file
.PHONY: version
version:
	sed -i "s/Version:.*$$/Version: $(VERSION).$(RELEASE)/" variantkey/DESCRIPTION

# Build the library
.PHONY: build
build: version
	cd variantkey \
	&& cp ../../LICENSE . \
	&& R -e 'renv::init();'
	R -e 'renv::restore();setwd("./variantkey");library(devtools);library(roxygen2);pkgbuild::build();'

.PHONY: test
test:
	R -e 'renv::restore();setwd("./variantkey");library(devtools);devtools::document();library(testthat);testthat::test_dir("tests")'

# Format the source code
.PHONY: format
format:
	astyle --style=allman --recursive --suffix=none 'variantkey/src/*.c'
	astyle --style=allman --recursive --suffix=none 'variantkey/src/*.h'
	#astyle --style=google --recursive --suffix=none 'variantkey/R/*.R'
	#astyle --style=google --recursive --suffix=none 'variantkey/tests/*.R'
	#astyle --style=google --recursive --suffix=none 'example/*.R'

# Remove any build artifact
.PHONY: clean
clean:
	rm -f variantkey.Rcheck variantkey/src/variantkey.o variantkey/src/variantkey.so
	rm -rf renv variantkey/renv variantkey/.Rbuildignore variantkey/.Rprofile .Rprofile

.PHONY: devdoc
devdoc:
	R -e 'renv::restore();library(devtools);setwd("./variantkey");devtools::document();'
