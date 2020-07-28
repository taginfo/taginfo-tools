#!/bin/sh

readonly SRC_DIR=$1
readonly BIN_DIR=$2
readonly TEST_ID=$3

check_count() {
    test `echo "SELECT count(*) FROM $1;" | $SQL` -eq $2
}

