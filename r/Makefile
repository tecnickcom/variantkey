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
#     sudo Rscript -e "install.packages(c('Rcpp', 'devtools', 'inline', 'pryr', 'renv', 'ragg', 'roxygen2', 'testthat', 'pkgdown'), repos = 'http://cran.us.r-project.org')"
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
	&& R -e 'renv::settings$$snapshot.type("implicit");renv::init();renv::snapshot();'
	R -e 'setwd("./variantkey");renv::restore();library(devtools);library(roxygen2);pkgbuild::build();'

.PHONY: test
test:
	R -e 'setwd("./variantkey");renv::restore();library(devtools);devtools::document();library(testthat);testthat::test_dir("tests")'

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
	rm -f variantkey.Rcheck variantkey/src/variantkey.o variantkey/src/variantkey.so variantkey/renv.lock
	rm -rf renv variantkey/renv variantkey/.Rbuildignore variantkey/.Rprofile .Rprofile

.PHONY: devdoc
devdoc:
	R -e 'setwd("./variantkey");renv::restore();library(devtools);devtools::document();'
