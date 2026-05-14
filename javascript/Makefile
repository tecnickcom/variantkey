# MAKEFILE
#
# @author      Nicola Asuni <info@tecnick.com>
# @link        https://github.com/tecnickcom/variantkey
# ------------------------------------------------------------------------------

# --- MAKE TARGETS ---

.PHONY: help
help:
	@echo ""
	@echo "$(PROJECT) Makefile."
	@echo "The following commands are available:"
	@echo ""
	@awk '/^## /{desc=substr($$0,4)} /^\.PHONY:/{if(NF>1) {target=$$2; if(desc) printf "  make %-15s: %s\n",target,desc; desc=""}}' Makefile
	@echo ""

all: clean format test build

## Run the unit tests
.PHONY: test
test:
	cd test && node test_variantkey.js '../src/variantkey.js'
	cd test && node test_regionkey.js '../src/variantkey.js'
	cd test && node test_esid.js '../src/variantkey.js'

## Build a minified version of the library
.PHONY: build
build: test
	@mkdir -p target/build
	uglifyjs --compress --keep-fnames --comments --output target/build/variantkey.js src/variantkey.js 

## Format the source code
.PHONY: format
format:
	js-beautify --replace src/variantkey.js
	js-beautify --replace test/test_variantkey.js

## Remove any build artifact
.PHONY: clean
clean:
	rm -rf target
