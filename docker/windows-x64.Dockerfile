# Cross-compilation image for the Windows x64 release leg of the CI matrix.
#
# GitHub's windows-latest runners don't ship gcc/make, and getting a real
# Windows machine to build the C library is unnecessary: mingw-w64 running
# on a plain Linux box cross-compiles the whole project (see ns_platform.h's
# PLATFORM_WINDOWS / os/windows split) without needing Windows at all. This
# mirrors the docker cross-compile approach sqlite-jdbc uses for its native
# builds (https://github.com/xerial/sqlite-jdbc/tree/master/docker).
FROM ubuntu:24.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        make \
        mingw-w64 \
        python3 \
        zip \
    && rm -rf /var/lib/apt/lists/*

# CC/AR aren't set here as ENV: Make's `:=` assignments in the Makefile
# take precedence over inherited environment variables, so the toolchain
# must be selected on the make command line instead, e.g.:
#   make CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar release-package-windows-cross

WORKDIR /work
