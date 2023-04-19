# MAKEFILE
#
# @author      Nicola Asuni <info@tecnick.com>
# @link        https://github.com/tecnickcom/variantkey
# ------------------------------------------------------------------------------

SHELL=/bin/bash
.SHELLFLAGS=-o pipefail -c

# Project owner
OWNER=Tecnick.com

# Project vendor
VENDOR=tecnickcom

# Lowercase VENDOR name for Docker
LCVENDOR=$(shell echo "${VENDOR}" | tr '[:upper:]' '[:lower:]')

# CVS path (path to the parent dir containing the project)
CVSPATH=github.com/${VENDOR}

# Project name
PROJECT=variantkey

# Project version
VERSION=$(shell cat VERSION)

# Project release number (packaging build number)
RELEASE=$(shell cat RELEASE)

# Current directory
CURRENTDIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))

# Target directory
TARGETDIR=$(CURRENTDIR)target

# --- MAKE TARGETS ---

# Display general help about this command
.PHONY: help
help:
	@echo ""
	@echo "VariantKey Makefile."
	@echo "The following commands are available:"
	@echo ""
	@echo "    make c            : Build and test the C version"
	@echo "    make go           : Build and test the GO version"
	@echo "    make javascript   : Build and test the Javascript version"
	@echo "    make python       : Build and test the Python version"
	@echo "    make python-class : Build and test the Python wrapper class"
	@echo "    make r            : Build and test the R version"
	@echo "    make clean        : Remove any build artifact"
	@echo "    make dbuild       : Build everything inside a Docker container"
	@echo "    make tag          : Tag the Git repository"
	@echo ""

all: clean c go javascript python python-class r

# Build and test the C version
.PHONY: c
c:
	cd c && make all

# Build and test the GO version
.PHONY: go
go:
	cd go && make all

# Build and test the Javascript version
.PHONY: javascript
javascript:
	cd javascript && make all

# Build and test the Python version
.PHONY: python
python:
	cd python && make all

# Build and test the Python wrapper class
.PHONY: python-class
python-class:
	cd python-class && make all

# Build and test the R version
.PHONY: r
r:
	cd r && make all

# Remove any build artifact
.PHONY: clean
clean:
	rm -rf target
	cd c && make clean
	cd go && make clean
	cd javascript && make clean
	cd python && make clean
	cd python-class && make clean
	cd r && make clean

# Build everything inside a Docker container
.PHONY: dbuild
dbuild:
	@mkdir -p target
	@rm -rf target/*
	@echo 0 > target/make.exit
	CVSPATH=$(CVSPATH) VENDOR=$(VENDOR) PROJECT=$(PROJECT) MAKETARGET='$(MAKETARGET)' ./dockerbuild.sh
	@exit `cat target/make.exit`

# Publish Documentation in GitHub (requires writing permissions)
.PHONY: pubdocs
pubdocs:
	rm -rf ./target/DOCS
	rm -rf ./target/gh-pages
	mkdir -p ./target/DOCS/c
	cp -r ./c/target/build/doc/html/* ./target/DOCS/c/
	# mkdir -p ./target/DOCS/go
	# cp -r ./go/target/docs/* ./target/DOCS/go/
	# mkdir -p ./target/DOCS/python
	# cp -r ./python/target/doc/variantkey.html ./target/DOCS/python/
	# mkdir -p ./target/DOCS/python-class
	# cp -r ./python-class/target/doc/*.html ./target/DOCS/python-class/
	# mkdir -p ./target/DOCS/r
	# cp -r ./r/variantkey/docs/* ./target/DOCS/r/
	# cp ./resources/doc/index.html ./target/DOCS/
	git clone git@github.com:tecnickcom/variantkey.git ./target/gh-pages
	cd target/gh-pages && git checkout gh-pages
	mv -f ./target/gh-pages/.git ./target/DOCS/
	rm -rf ./target/gh-pages
	cd ./target/DOCS/ && \
	git add . -A && \
	git commit -m 'Update documentation' && \
	git push origin gh-pages --force

# Tag the Git repository
.PHONY: tag
tag:
	git tag -a "v$(VERSION)" -m "Version $(VERSION)" && \
	git push origin --tags
