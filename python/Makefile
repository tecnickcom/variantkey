# MAKEFILE
#
# @author      Nicola Asuni <info@tecnick.com>
# @link        https://github.com/tecnickcom/variantkey
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

# Path fot the python binary
PYTHON=$(shell which python3)

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

all: clean venv build test

## Build the package
.PHONY: build
build: version
	source venv/bin/activate \
	&& python -m build --sdist --wheel

## Remove any build artifact
.PHONY: clean
clean:
	rm -rf venv target c Dockerfile htmlcov build dist .pytest_cache .cache .benchmarks ./test/*.so ./test/__pycache__ ./variantkey/__pycache__ ./variantkey.egg-info
	find . -type f -name '*.pyc' -exec rm -f {} \;

## Generate source code documentation
.PHONY: docs
docs:
	pydoc3 -p 1234 $(PROJECT)

## Format the source code
.PHONY: format
format:
	astyle --style=allman --recursive --suffix=none 'variantkey/*.h'
	astyle --style=allman --recursive --suffix=none 'variantkey/*.c'
	black setup.py

## Unit tests
.PHONY: test
test:
	source venv/bin/activate \
	&& pytest

## Set the version from VERSION file
.PHONY: version
version:
	sed $(SEDINPLACE) "s/version=.*,$$/version=\"$(VERSION).$(RELEASE)\",/" setup.py

## Create a virtual environment
.PHONY: venv
venv: clean version
	rm -rf venv
	virtualenv -p $(PYTHON) venv
	source venv/bin/activate \
	&& python --version \
	&& pip install --upgrade pip \
	&& pip install --upgrade --pre wheel build black pytest setuptools \
	&& pip install --use-pep517 --no-build-isolation -e .[test]
