#!/usr/bin/env bash

rm -rf release
mkdir -p release
python3 src/scripts/amalgamate.py
mv numstore.c release 
cp src/numstore.h release 
cp src/smartfiles.h release 
cp README.md release
cp docs/index.md release/user_guide.md

zip -rX release-v1.2.0.zip release/
tar -czf release-v1.2.0.tar.gz release/
