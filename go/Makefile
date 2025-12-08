# MAKEFILE
#
# @author      Nicola Asuni <info@tecnick.com>
# @link        https://github.com/tecnickcom/variantkey
# ------------------------------------------------------------------------------

SHELL=/bin/bash
.SHELLFLAGS=-o pipefail -c

# Project owner
OWNER=tecnickcom

# Project vendor
VENDOR=${OWNER}

# CVS path (path to the parent dir containing the project)
CVSPATH=github.com/${VENDOR}

# Project name
PROJECT=variantkey

# Project version
VERSION=$(shell cat ../VERSION)

# Project release number (packaging build number)
RELEASE=$(shell cat ../RELEASE)

# Current directory
CURRENTDIR=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))

# Target directory
TARGETDIR=$(CURRENTDIR)target

# Directory where to store binary utility tools
BINUTIL=$(TARGETDIR)/binutil

# GO lang path
ifeq ($(GOPATH),)
	# extract the GOPATH
	GOPATH=$(firstword $(subst /src/, ,$(CURRENTDIR)))
endif

# Add the GO binary dir in the PATH
export PATH := $(GOPATH)/bin:$(PATH)

# Common commands
GO=GOPATH=$(GOPATH) GOPRIVATE=$(CVSPATH) $(shell which go)
GOVERSION=${shell go version | grep -Po '(go[0-9]+.[0-9]+)'}
GOFMT=$(shell which gofmt)
GOTEST=$(GO) test
GODOC=GOPATH=$(GOPATH) $(shell which godoc)
GOLANGCILINT=$(BINUTIL)/golangci-lint
GOLANGCILINTVERSION=v2.7.2

# Directory containing the source code
SRCDIR=./src

# Enable junit report when not in LOCAL mode
ifeq ($(strip $(DEVMODE)),LOCAL)
	TESTEXTRACMD=&& $(GO) tool cover -func=$(TARGETDIR)/report/coverage.out
else
	TESTEXTRACMD=2>&1 | tee >(PATH=$(GOPATH)/bin:$(PATH) go-junit-report > $(TARGETDIR)/test/report.xml); test $${PIPESTATUS[0]} -eq 0
endif

# --- MAKE TARGETS ---

# Display general help about this command
.PHONY: help
help:
	@echo ""
	@echo "$(PROJECT) Makefile."
	@echo "GOPATH=$(GOPATH)"
	@echo "The following commands are available:"
	@echo ""
	@echo "  make clean      : Remove any build artifact"
	@echo "  make coverage   : Generate the coverage report"
	@echo "  make deps       : Get dependencies"
	@echo "  make format     : Format the source code"
	@echo "  make linter     : Check code against multiple linters"
	@echo "  make mod        : Download dependencies"
	@echo "  make qa         : Run all tests and static analysis tools"
	@echo "  make test       : Run unit tests"
	@echo "  make updateall  : Update everything"
	@echo "  make updatego   : Update Go version"
	@echo "  make updatelint : Update golangci-lint version"
	@echo "  make updatemod  : Update dependencies"
	@echo ""
	@echo "Use DEVMODE=LOCAL for human friendly output."
	@echo ""
	@echo "To test and build everything from scratch:"
	@echo "    DEVMODE=LOCAL make format clean mod deps qa"
	@echo "or use the shortcut:"
	@echo "    make x"
	@echo ""

all: x

# Alias to test and build everything from scratch
.PHONY: x
x:
	DEVMODE=LOCAL $(MAKE) format clean mod deps qa

# Remove any build artifact
.PHONY: clean
clean:
	rm -rf $(TARGETDIR)

# Generate the coverage report
.PHONY: coverage
coverage: ensuretarget
	$(GO) tool cover -html=$(TARGETDIR)/report/coverage.out -o $(TARGETDIR)/report/coverage.html

# Get the test dependencies
.PHONY: deps
deps: ensuretarget
	curl --silent --show-error --fail --location https://raw.githubusercontent.com/golangci/golangci-lint/master/install.sh | sh -s -- -b $(BINUTIL) $(GOLANGCILINTVERSION)

# Create the trget directories if missing
.PHONY: ensuretarget
ensuretarget:
	@mkdir -p $(TARGETDIR)/test
	@mkdir -p $(TARGETDIR)/report
	@mkdir -p $(TARGETDIR)/binutil
	@mkdir -p $(TARGETDIR)/docs

# build example
.PHONY: example
example:
	$(GO) build -o $(TARGETDIR)/example $(CURRENTDIR)/example \
	&& $(TARGETDIR)/example

# Format the source code
.PHONY: format
format:
	@find $(SRCDIR) -type f -name "*.go" -exec bash -c "$(GOFMT) -s -w {}" \;

# Execute multiple linter tools
.PHONY: linter
linter:
	$(GOLANGCILINT) run $(SRCDIR)/...

.PHONY: mod
mod: gotools
	$(GO) mod download all

# Run all tests and static analysis tools
.PHONY: qa
qa: linter test coverage

# Run the unit tests
.PHONY: test
test: ensuretarget
	@echo -e "\n\n>>> START: Unit Tests <<<\n\n"
	$(GOTEST) \
	-shuffle=on \
	-tags=unit \
	-covermode=atomic \
	-bench=. \
	-race \
	-failfast \
	-coverprofile=target/report/coverage.out \
	-v $(SRCDIR) $(TESTEXTRACMD)
	@echo -e "\n\n>>> END: Unit Tests <<<\n\n"

# Get the go tools
.PHONY: gotools
gotools:
	$(GO) install github.com/jstemmer/go-junit-report/v2@latest

# Update everything
.PHONY: updateall
updateall: updatego updatelint updatemod

# Update go version
.PHONY: updatego
updatego:
	$(eval LAST_GO_TOOLCHAIN=$(shell curl -s https://go.dev/dl/ | grep -oP 'go[0-9]+\.[0-9]+\.[0-9]+\.linux-amd64\.tar\.gz' | head -n 1 | grep -oP 'go[0-9]+\.[0-9]+\.[0-9]+'))
	$(eval LAST_GO_VERSION=$(shell echo ${LAST_GO_TOOLCHAIN} | grep -oP '[0-9]+\.[0-9]+'))
	sed -i "s|^go [0-9]*\.[0-9]*$$|go ${LAST_GO_VERSION}|g" ../go.mod
	sed -i "s|^toolchain go[0-9]*\.[0-9]*\.[0-9]*$$|toolchain ${LAST_GO_TOOLCHAIN}|g" ../go.mod

# Update linter version
.PHONY: updatelint
updatelint:
	$(eval LAST_GOLANGCILINT_VERSION=$(shell curl -sL https://github.com/golangci/golangci-lint/releases/latest | grep -oP '<title>Release \Kv[0-9]+\.[0-9]+\.[0-9]+'))
	sed -i "s|^GOLANGCILINTVERSION=v[0-9]*\.[0-9]*\.[0-9]*$$|GOLANGCILINTVERSION=${LAST_GOLANGCILINT_VERSION}|g" Makefile

# Update dependencies
.PHONY: updatemod
updatemod: mod
	$(GO) get -t -u ./... && \
	$(GO) mod tidy -compat=$(shell grep -oP 'go \K[0-9]+\.[0-9]+' ../go.mod)
