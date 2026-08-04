#!/usr/bin/env bash

rm -rf release
mkdir -p release
python3 src/scripts/amalgamate.py
mv numstore.c release 
cp src/numstore.h release 
cp src/smartfiles.h release 
cp docs/release.md release/COMPILING.md
cp docs/index.md release/USER_GUDE.md
cp src/samples/* release

zip -rX release-v1.2.0.zip release/
tar -czf release-v1.2.0.tar.gz release/
