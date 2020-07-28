#!/bin/sh
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

DATA=${SRC_DIR}/test/data.opl
DB=stats.db

rm -f $DB
sqlite3 $DB <${SRC_DIR}/test/init.sql
sqlite3 $DB <${SRC_DIR}/test/pre.sql
${BIN_DIR}/src/taginfo-stats $DATA $DB

test "" = "$(sqlite3 $DB 'SELECT key1, key2, count_all1, count_all2, similarity FROM similar_keys ORDER BY key1, key2')"

${BIN_DIR}/src/taginfo-similarity $DB

test "highway|highwy|1" = $(sqlite3 $DB 'SELECT key1, key2, similarity FROM similar_keys ORDER BY key1, key2')

#-----------------------------------------------------------------------------
