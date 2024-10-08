# MAKEFILE
#
# @author      Nicola Asuni <info@tecnick.com>
# @link        https://github.com/tecnickcom/variantkey
# ------------------------------------------------------------------------------

# --- MAKE TARGETS ---

# Display general help about this command
.PHONY: help
help:
	@echo ""
	@echo "VariantKey Javascript Makefile."
	@echo "Requires: node-js and uglify-js."
	@echo "The following commands are available:"
	@echo ""
	@echo "  make test   : Run the unit tests against source code"
	@echo "  make build  : Build and test a minified version of the library"
	@echo "  make format : Format the source code"
	@echo "  make clean  : Remove any build artifact"
	@echo ""

all: clean format test build

# Run the unit tests
.PHONY: test
test:
	cd test && node test_variantkey.js '../src/variantkey.js'
	cd test && node test_regionkey.js '../src/variantkey.js'
	cd test && node test_esid.js '../src/variantkey.js'

# Build a minified version of the library
.PHONY: build
build: test
	@mkdir -p target/build
	uglifyjs --compress --keep-fnames --comments --output target/build/variantkey.js src/variantkey.js 

# Format the source code
.PHONY: format
format:
	js-beautify --replace src/variantkey.js
	js-beautify --replace test/test_variantkey.js

# Remove any build artifact
.PHONY: clean
clean:
	rm -rf target
