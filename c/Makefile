# MAKEFILE
#
# @author      Nicola Asuni <info@tecnick.com>
# @link        https://github.com/tecnickcom/variantkey
# ------------------------------------------------------------------------------

# Use bash as shell (Note: Ubuntu now uses dash which doesn't support PIPESTATUS).
SHELL=/bin/bash
.SHELLFLAGS=-o pipefail -c

# CVS path (path to the parent dir containing the project)
CVSPATH=github.com/tecnickcom

# Project owner
OWNER=Tecnick.com

# Project vendor
VENDOR=tecnickcom

# Project name
PROJECT=variantkey

# Project version
VERSION=$(shell cat ../VERSION)

# Project release number (packaging build number)
RELEASE=$(shell cat ../RELEASE)

# Current directory
CURRENTDIR=$(CURDIR)/

# Name of RPM or DEB package
PKGNAME=${VENDOR}-${PROJECT}-headers

# Path for binary files (where the executable files will be installed)
LIBPATH=usr/include/$(VENDOR)/$(PROJECT)/

# Path path for documentation
DOCPATH=usr/share/doc/$(PKGNAME)/

# Installation path for the binary files
PATHINSTLIB=$(DESTDIR)/$(LIBPATH)

# Installation path for documentation
PATHINSTDOC=$(DESTDIR)/$(DOCPATH)

# RPM Packaging path (where RPMs will be stored)
PATHRPMPKG=$(CURRENTDIR)/target/RPM

# DEB Packaging path (where DEBs will be stored)
PATHDEBPKG=$(CURRENTDIR)/target/DEB

# sed argument for in-place substitutions
SEDINPLACE=-i
ifeq ($(shell uname -s),Darwin)
	SEDINPLACE=-i ''
endif

# Optional cmake settings.
# Each option is only passed when the corresponding variable is set: passing an
# empty value (e.g. -DCMAKE_INSTALL_PREFIX=) clobbers the CMake default instead
# of leaving it alone. Values are quoted to survive paths containing spaces.
CMAKEOPTS=\
$(if $(CMAKE_C_FLAGS),-DCMAKE_C_FLAGS="$(CMAKE_C_FLAGS)") \
$(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE="$(CMAKE_TOOLCHAIN_FILE)") \
$(if $(CMAKE_INSTALL_PATH),-DCMAKE_INSTALL_PREFIX="$(CMAKE_INSTALL_PATH)")

# lcov options: branch coverage is required by the coverage gate.
LCOVOPTS=--rc branch_coverage=1

# clang-tidy check list, shared by the tidy and linter targets.
TIDYCHECKS=*,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,-readability-function-cognitive-complexity,-altera-struct-pack-align,-altera-id-dependent-backward-branch,-bugprone-easily-swappable-parameters,-altera-unroll-loops,-readability-isolate-declaration,-llvmlibc-restrict-system-libc-headers,-readability-identifier-length,-cppcoreguidelines-avoid-magic-numbers,-readability-magic-numbers,-llvm-header-guard,-llvm-include-order,-android-cloexec-open,-hicpp-no-assembler,-hicpp-signed-bitwise,-hicpp-use-nullptr,-modernize-use-nullptr,-misc-include-cleaner,-clang-analyzer-alpha.*

# Extra clang-tidy exclusions for the test files imported from the binsearch
# repository: upstream only runs clang-tidy on the header, so its tests are not
# written against this check list and they must not be edited here, otherwise
# the next "make binsearch" would revert the changes.
VENDORTIDYCHECKS=-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp,-cppcoreguidelines-avoid-non-const-global-variables,-bugprone-not-null-terminated-result,-android-cloexec-fopen

# Test files imported from the binsearch repository.
VENDORTESTS=$(wildcard test/test_binsearch*.c)

# Test files owned by this repository.
OWNTESTS=$(filter-out $(VENDORTESTS),$(wildcard test/*.c))

# Git tag, branch or commit of the imported binsearch code.
BINSEARCHVERSION=v9.5.0

# Base URL of the raw binsearch files at the selected version.
BINSEARCHURL=https://raw.githubusercontent.com/tecnickcom/binsearch/$(BINSEARCHVERSION)

# Test files to import from the binsearch repository (the list must be extended
# when a new test file is added upstream; the build registers them by glob).
BINSEARCHTESTS=test_binsearch.c test_binsearch_col.c test_binsearch_file.c test_binsearch_mmap.c test_binsearch_range.c

# Test data files to import from the binsearch repository.
BINSEARCHDATA=test_data.bin test_data.hex test_data_arrow.bin test_data_binsrc.bin test_data_binsrc.hex test_data_col.bin test_data_col.hex test_data_feather.bin

# Temporary directory for the downloaded binsearch files.
BINSEARCHDIR=target/binsearch

# --- MAKE TARGETS ---

.PHONY: help
help:
	@echo ""
	@echo "$(PROJECT) Makefile."
	@echo "The following commands are available:"
	@echo ""
	@awk '/^## /{desc=substr($$0,4)} /^\.PHONY:/{if(NF>1) {target=$$2; if(desc) printf "  make %-15s: %s\n",target,desc; desc=""}}' Makefile
	@echo ""

all: clean version format test coverage sanitize linter build rpm deb package_vk doc

## Test C code compatibility with C++
.PHONY: testcpp
testcpp:
	# xargs is required to propagate the compiler exit status: with
	# "find -exec ... \;" find always exits 0.
	find ./src/variantkey -type f -name '*.h' -print0 | xargs -0 -n 1 gcc -c -pedantic -Werror -Wall -Wextra -Wcast-align -Wundef -Wformat-security -std=c++23 -x c++ -o /dev/null

## Build and run the unit tests
.PHONY: test
test: testcpp
	@echo -e "\n\n*** BUILD TEST ***\n"
	rm -rf target/test
	mkdir -p target/test/coverage
	cd target/test && \
	cmake $(CMAKEOPTS) \
	-DCMAKE_BUILD_TYPE=Coverage \
	../.. | tee cmake.log ; test $${PIPESTATUS[0]} -eq 0 && \
	export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:./ && \
	make | tee make.log ; test $${PIPESTATUS[0]} -eq 0 && \
	lcov --zerocounters --directory . && \
	lcov $(LCOVOPTS) --capture --initial --directory . --output-file coverage/variantkey.cov && \
	env CTEST_OUTPUT_ON_FAILURE=1 make test | tee test.log ; test $${PIPESTATUS[0]} -eq 0 && \
	lcov $(LCOVOPTS) --no-checksum --directory . --capture --output-file coverage/variantkey.info && \
	lcov $(LCOVOPTS) --ignore-errors unused --remove coverage/variantkey.info '*/test/*' '/usr/include/*' --output-file coverage/variantkey.info && \
	genhtml $(LCOVOPTS) -o coverage -t "VariantKey Test Coverage" coverage/variantkey.info
ifeq ($(VH_BUILD_DOXYGEN),ON)
	cd target && \
	make doc | tee doc.log ; test $${PIPESTATUS[0]} -eq 0
endif

## Check that the lcov line, function and branch coverage from the test target is 100%
.PHONY: coverage
coverage: test
	@summary=$$(lcov $(LCOVOPTS) --summary target/test/coverage/variantkey.info 2>&1); \
	for metric in lines functions branches; do \
		value=$$(echo "$$summary" | grep -oE "$$metric[.]*: [0-9]+\.[0-9]+%" | grep -oE '[0-9]+\.[0-9]+' | head -1); \
		echo "$$metric coverage: $$value%"; \
		if [ "$$value" != "100.0" ]; then echo "ERROR: $$metric coverage '$$value%' is below 100%"; exit 1; fi; \
	done

## Build and run the unit tests under the Address and Undefined Behaviour sanitizers
.PHONY: sanitize
sanitize:
	@mkdir -p target/sanitize
	for src in test/test_*.c; do \
		name=$$(basename "$$src" .c); \
		$(CC) -std=c17 -pedantic -Wall -Wextra -Wcast-align -Wundef -Wformat -Wformat-security -Wshadow \
		-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
		-o "target/sanitize/$$name" "$$src" || exit 1; \
	done
	cp test/data/*.bin target/sanitize/
	cd target/sanitize && for src in ../../test/test_*.c; do \
		name=$$(basename "$$src" .c); \
		echo "*** $$name ***"; \
		"./$$name" || exit 1; \
	done

## Tidy the code via clang-tidy
.PHONY: tidy
tidy:
	# The compile flags are passed after "--", so clang-tidy analyses the headers
	# with the right language and standard. --warnings-as-errors makes the target
	# fail on any reported warning.
	clang-tidy src/variantkey/*.h vk/vk.c --header-filter='.*/src/variantkey/.*\.h' --warnings-as-errors='$(TIDYCHECKS)' -checks='$(TIDYCHECKS)' -- -x c -std=c17 -I src/variantkey
	clang-tidy $(OWNTESTS) test/rsidvar_bench/*.c --header-filter='.*/src/variantkey/.*\.h' --warnings-as-errors='$(TIDYCHECKS),-concurrency-mt-unsafe' -checks='$(TIDYCHECKS),-concurrency-mt-unsafe' -- -x c -std=c17 -I src/variantkey
	clang-tidy $(VENDORTESTS) --header-filter='.*/src/variantkey/.*\.h' --warnings-as-errors='$(TIDYCHECKS),-concurrency-mt-unsafe,$(VENDORTIDYCHECKS)' -checks='$(TIDYCHECKS),-concurrency-mt-unsafe,$(VENDORTIDYCHECKS)' -- -x c -std=c17 -I src/variantkey

## Check the code via cppcheck and clang-tidy
.PHONY: linter
linter: tidy
	cppcheck --enable=warning,style,performance,portability --check-level=exhaustive --error-exitcode=1 --inline-suppr --std=c17 -I src/variantkey src/variantkey/*.h vk/vk.c

## Build the library
.PHONY: build
build:
	@echo -e "\n\n*** BUILD RELEASE ***\n"
	rm -rf target/build
	mkdir -p target/build
	cd target/build && \
	cmake $(CMAKEOPTS) \
	-DCMAKE_BUILD_TYPE=Release \
	../.. | tee cmake.log ; test $${PIPESTATUS[0]} -eq 0 && \
	export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:./ && \
	make | tee make.log ; test $${PIPESTATUS[0]} -eq 0
	cd target/build && \
	export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:./ && \
	env CTEST_OUTPUT_ON_FAILURE=1 make test | tee build.log ; test $${PIPESTATUS[0]} -eq 0

## Package vk tool
.PHONY: package_vk
package_vk: version
	cd target/build/vk && make -j package

## Set the version from VERSION file
.PHONY: version
version:
	sed $(SEDINPLACE) "s/PROJECT_NUMBER.*=.*$$/PROJECT_NUMBER = $(VERSION)/" doc/Doxyfile

## Generate source code documentation
.PHONY: doc
doc:
	cd target/build && \
	make doc | tee doc.log ; test $${PIPESTATUS[0]} -eq 0

## Import the binsearch header, tests and test data from the upstream repository
.PHONY: binsearch
binsearch:
	rm -rf "$(BINSEARCHDIR)"
	mkdir -p "$(BINSEARCHDIR)"
	# Every file is downloaded before anything is replaced, so that a failed
	# download cannot leave a partially updated source tree. The -f option makes
	# curl fail on a HTTP error instead of saving the error page.
	curl -sSLf -o "$(BINSEARCHDIR)/binsearch.h" "$(BINSEARCHURL)/c/src/binsearch/binsearch.h"
	for file in $(BINSEARCHTESTS); do \
		curl -sSLf -o "$(BINSEARCHDIR)/$$file" "$(BINSEARCHURL)/c/test/$$file" || exit 1; \
	done
	for file in $(BINSEARCHDATA); do \
		curl -sSLf -o "$(BINSEARCHDIR)/$$file" "$(BINSEARCHURL)/c/test/data/$$file" || exit 1; \
	done
	# The header is installed with the variantkey ones, so the include guard and
	# the include path used by the tests are renamed. This is the only change
	# applied to the imported files: anything else must be fixed upstream.
	sed -e 's|BINSEARCH_BINSEARCH_H|VARIANTKEY_BINSEARCH_H|g' \
	"$(BINSEARCHDIR)/binsearch.h" > src/variantkey/binsearch.h
	rm -f test/test_binsearch*.c
	for file in $(BINSEARCHTESTS); do \
		sed -e 's|\.\./src/binsearch/binsearch\.h|../src/variantkey/binsearch.h|' "$(BINSEARCHDIR)/$$file" > "test/$$file" || exit 1; \
	done
	for file in $(BINSEARCHDATA); do \
		cp -f "$(BINSEARCHDIR)/$$file" "test/data/$$file" || exit 1; \
	done
	rm -rf "$(BINSEARCHDIR)"
	@echo -e "\nImported binsearch $(BINSEARCHVERSION) from $(BINSEARCHURL)\n"

## Format the source code
.PHONY: format
format:
	astyle --style=allman --recursive --suffix=none 'src/variantkey/*.h'
	astyle --style=allman --recursive --suffix=none 'test/*.c'
	astyle --style=allman --recursive --suffix=none 'vk/*.c'

## Remove any build artifact
.PHONY: clean
clean:
	rm -rf target

## Install this application
.PHONY: install
install: uninstall
	mkdir -p "$(PATHINSTLIB)"
	cp src/variantkey/*.h "$(PATHINSTLIB)"
	find "$(PATHINSTLIB)" -type f -exec chmod 644 {} \;
	mkdir -p "$(PATHINSTDOC)"
	cp -f ../LICENSE "$(PATHINSTDOC)"
	cp -f ../README.md "$(PATHINSTDOC)"
	cp -f ../VERSION "$(PATHINSTDOC)"
	cp -f ../RELEASE "$(PATHINSTDOC)"
	chmod -R 644 "$(PATHINSTDOC)"*

## Remove all installed files (excluding configuration files)
.PHONY: uninstall
uninstall:
	rm -rf "$(PATHINSTLIB)"
	rm -rf "$(PATHINSTDOC)"

## Build the RPM package for RedHat-like Linux distributions
.PHONY: rpm
rpm:
	@test $(words $(CURDIR)) -eq 1 || { echo "ERROR: rpmbuild does not support spaces in the project path: $(CURDIR)"; exit 1; }
	rm -rf "$(PATHRPMPKG)"
	rpmbuild \
	--define "_topdir $(PATHRPMPKG)" \
	--define "_vendor $(VENDOR)" \
	--define "_owner $(OWNER)" \
	--define "_project $(PROJECT)" \
	--define "_package $(PKGNAME)" \
	--define "_version $(VERSION)" \
	--define "_release $(RELEASE)" \
	--define "_current_directory $(CURRENTDIR)" \
	--define "_libpath /$(LIBPATH)" \
	--define "_docpath /$(DOCPATH)" \
	-bb resources/rpm/rpm.spec

## Build the DEB package for Debian-like Linux distributions
.PHONY: deb
deb:
	rm -rf "$(PATHDEBPKG)"
	make install DESTDIR="$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)"
	rm -f "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/$(DOCPATH)LICENSE"
	tar -zcvf "$(PATHDEBPKG)/$(PKGNAME)_$(VERSION).orig.tar.gz" -C "$(PATHDEBPKG)/" "$(PKGNAME)-$(VERSION)"
	cp -rf ./resources/debian "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian"
	find "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian/" -type f -exec sed $(SEDINPLACE) "s/~#DATE#~/`date -R`/" {} \;
	find "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian/" -type f -exec sed $(SEDINPLACE) "s/~#PKGNAME#~/$(PKGNAME)/" {} \;
	find "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian/" -type f -exec sed $(SEDINPLACE) "s/~#VERSION#~/$(VERSION)/" {} \;
	find "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian/" -type f -exec sed $(SEDINPLACE) "s/~#RELEASE#~/$(RELEASE)/" {} \;
	echo "$(LIBPATH)" > "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian/$(PKGNAME).dirs"
	echo "$(LIBPATH)* $(LIBPATH)" > "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian/install"
	echo "$(DOCPATH)" >> "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian/$(PKGNAME).dirs"
	echo "$(DOCPATH)* $(DOCPATH)" >> "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian/install"
	echo "new-package-should-close-itp-bug" >> "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)/debian/$(PKGNAME).lintian-overrides"
	cd "$(PATHDEBPKG)/$(PKGNAME)-$(VERSION)" && debuild -us -uc
