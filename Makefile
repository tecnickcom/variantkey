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
CURRENTDIR=$(CURDIR)/

# Target directory
TARGETDIR=target

# Docker command
ifeq ($(DOCKER),)
	DOCKER=docker
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

all: c go javascript python python-class r

## Build and test the C version
.PHONY: c
c:
	cd c && make all

## Import the binsearch header, tests and test data from the upstream repository
.PHONY: binsearch
binsearch:
	cd c && make binsearch

## Build and test the GO version
.PHONY: go
go:
	cd go && make all

## Build and test the Javascript version
.PHONY: javascript
javascript:
	cd javascript && make all

## Build and test the Python version
.PHONY: python
python:
	cd python && make all

## Build and test the Python wrapper class
.PHONY: python-class
python-class:
	cd python-class && make all

## Build and test the R version
.PHONY: r
r:
	cd r && make all

## Run the unit tests of every language version
.PHONY: test
test:
	# NOTE: the python, python-class and r versions need their environment to be
	# provisioned first ("make venv" / renv::restore), which is what the
	# per-language "all" targets do. Use "make all" for a cold checkout.
	cd c && make test
	cd go && make test
	cd javascript && make test
	cd python && make test
	cd python-class && make test
	cd r && make test

## Remove any build artifact
.PHONY: clean
clean:
	rm -rf target
	cd c && make clean
	cd go && make clean
	cd javascript && make clean
	cd python && make clean
	cd python-class && make clean
	cd r && make clean

## Build everything inside a Docker container
.PHONY: dbuild
dbuild: dockerdev
	@mkdir -p "$(TARGETDIR)"
	@rm -rf "$(TARGETDIR)/"*
	@echo 0 > "$(TARGETDIR)/make.exit"
	CVSPATH=$(CVSPATH) VENDOR=$(LCVENDOR) PROJECT=$(PROJECT) MAKETARGET='$(MAKETARGET)' "$(CURRENTDIR)dockerbuild.sh"
	@exit `cat $(TARGETDIR)/make.exit`

## Build a base development Docker image
.PHONY: dockerdev
dockerdev:
	$(DOCKER) build --pull --tag "${LCVENDOR}/dev_${PROJECT}" --file ./resources/docker/Dockerfile.dev ./resources/docker/

## Publish Documentation in GitHub (requires writing permissions)
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

## Tag the Git repository
.PHONY: tag
tag:
	git tag -a "v$(VERSION)" -m "Version $(VERSION)" && \
	git push origin --tags

## Increase the patch number in the VERSION file
.PHONY: versionup
versionup:
	echo ${VERSION} | gawk -F. '{printf("%d.%d.%d\n",$$1,$$2,(($$3+1)));}' > VERSION
