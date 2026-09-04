#!/usr/bin/env bash 

DBNAME="test"
DURATION=5 
SEED=12345 
COMMIT_HASH=$(git rev-parse HEAD)
SEQ_ID=0

./build/debug/target/bin/numstore_simulation_test \
  $DBNAME  \
  $DURATION \
  $SEED \
  $COMMIT_HASH \
  $SEQ_ID 
