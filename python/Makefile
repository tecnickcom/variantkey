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

# --- MAKE TARGETS ---

# Display general help about this command
.PHONY: help
help:
	@echo ""
	@echo "${PROJECT} Python Makefile."
	@echo "The following commands are available:"
	@echo ""
	@echo "    make build   : Build and test the library"
	@echo "    make clean   : Remove any build artifact"
	@echo "    make docs    : Generate documentation"
	@echo "    make format  : Format the source code"
	@echo "    make version : Set the version from VERSION file"
	@echo "    make vbuild  : Build and test the library inside a virtualenv"
	@echo ""

all: clean vbuild

# Build and test the package
.PHONY: build
build: clean version
	pip install -e .[test] && python setup.py sdist bdist_wheel install test

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

# Set the version from VERSION file
.PHONY: version
version:
	sed -i "s/version=.*,$$/version=\"$(VERSION).$(RELEASE)\",/" setup.py

# Build and test the package
.PHONY: vbuild
vbuild: clean version
	rm -rf venv
	virtualenv -p /usr/bin/python3 venv
	source venv/bin/activate \
	&& python --version \
	&& pip install --pre black build pytest wheel \
	&& pip install -e .[test] \
	&& pytest \
	&& python -m build --sdist --wheel
