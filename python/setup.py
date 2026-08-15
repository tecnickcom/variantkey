#!/usr/bin/env python

import os
import shutil
from codecs import open
from os.path import dirname, exists, join
from subprocess import call
from setuptools import setup, find_packages, Extension, Command

HERE = dirname(__file__)

# Directory holding the C headers *inside* this package. An sdist cannot contain
# anything above its own root, so the headers, the README and the LICENSE are
# staged here from the repository before the sdist is built. Everything below
# then refers to package-relative paths only, which is what makes a build from
# the source distribution work. See MANIFEST.in, and python/.gitignore which
# deliberately ignores these staged copies.
HEADERS_DIR = join("c", "src", "variantkey")

HEADERS = [
    "binsearch.h",
    "esid.h",
    "genoref.h",
    "hex.h",
    "nrvk.h",
    "regionkey.h",
    "rsidvar.h",
    "variantkey.h",
]


def stage_repo_files():
    """Copy the files shared with the rest of the repository into this package.

    Only runs from a git checkout, where the "../c" tree exists. From an unpacked
    sdist the copies are already in place and there is nothing above the root to
    copy from.
    """
    src_headers = join(HERE, "..", "c", "src", "variantkey")
    if not exists(src_headers):
        return  # building from an sdist: the staged copies are already here
    dst_headers = join(HERE, HEADERS_DIR)
    os.makedirs(dst_headers, exist_ok=True)
    for name in HEADERS:
        shutil.copyfile(join(src_headers, name), join(dst_headers, name))
    for name in ("README.md", "LICENSE"):
        shutil.copyfile(join(HERE, "..", name), join(HERE, name))


stage_repo_files()


def read(fname):
    return open(join(dirname(__file__), fname)).read()


class RunTests(Command):
    """Run all tests."""

    description = "run tests"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        """Run all tests!"""
        errno = call(["py.test", "--verbose"])
        raise SystemExit(errno)


setup(
    name="variantkey",
    version="5.8.0.0",
    keywords=("variantkey variant key genetic genomics"),
    description="VariantKey Bindings for Python",
    long_description=read("README.md"),
    author="Nicola Asuni",
    author_email="info@tecnick.com",
    url="https://github.com/tecnickcom/variantkey",
    license="MIT",
    platforms="Linux",
    packages=find_packages(exclude=["doc", "test*"]),
    ext_modules=[
        Extension(
            "variantkey",
            ["variantkey/pyvariantkey.c"],
            include_dirs=[HEADERS_DIR, "variantkey"],
            extra_compile_args=[
                "-O3",
                "-pedantic",
                "-std=c2x",
                "-Wall",
                "-Wextra",
                "-Wno-strict-prototypes",
                "-Wunused-value",
                "-Wcast-align",
                "-Wundef",
                "-Wformat",
                "-Wformat-security",
                "-Wshadow",
                "-Wno-format-overflow",
            ],
        )
    ],
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Programming Language :: C",
        "Programming Language :: Python",
    ],
    extras_require={
        "test": [
            "coverage",
            "pytest",
            "pytest-benchmark",
            "pytest-cov",
            "pycodestyle",
            "black",
        ]
    },
    cmdclass={"test": RunTests},
)
