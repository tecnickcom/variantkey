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

# --- MAKE TARGETS ---

# Display general help about this command
.PHONY: help
help:
	@echo ""
	@echo "${PROJECT} Python Makefile."
	@echo "The following commands are available:"
	@echo ""
	@echo "  make build   : Build the library"
	@echo "  make clean   : Remove any build artifact"
	@echo "  make docs    : Generate documentation"
	@echo "  make format  : Format the source code"
	@echo "  make test    : Test the library"
	@echo "  make venv    : Create a virtual environment"
	@echo "  make version : Set the version from VERSION file"
	@echo ""

all: clean venv build test

# Build the package
.PHONY: build
build: version
	source venv/bin/activate \
	&& python -m build --sdist --wheel

# Remove any build artifact
.PHONY: clean
clean:
	rm -rf venv target c Dockerfile htmlcov build dist .pytest_cache .cache .benchmarks ./test/*.so ./test/__pycache__ ./variantkey/__pycache__ ./variantkey.egg-info
	find . -type f -name '*.pyc' -exec rm -f {} \;

# Generate source code documentation
.PHONY: docs
docs:
	pydoc3 -p 1234 $(PROJECT)

# Format the source code
.PHONY: format
format:
	astyle --style=allman --recursive --suffix=none 'variantkey/*.h'
	astyle --style=allman --recursive --suffix=none 'variantkey/*.c'
	black setup.py

.PHONY: test
test:
	source venv/bin/activate \
	&& pytest

# Set the version from VERSION file
.PHONY: version
version:
	sed -i "s/version=.*,$$/version=\"$(VERSION).$(RELEASE)\",/" setup.py

# Create a virtual environment
.PHONY: venv
venv: clean version
	rm -rf venv
	virtualenv -p $(PYTHON) venv
	source venv/bin/activate \
	&& python --version \
	&& pip install --upgrade pip \
	&& pip install --upgrade --pre wheel build black pytest setuptools \
	&& pip install --use-pep517 --no-build-isolation -e .[test]
