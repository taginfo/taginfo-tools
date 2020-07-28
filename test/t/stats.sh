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

test 'db' = $(sqlite3 $DB 'SELECT id FROM source')

sqlite3 $DB 'SELECT key, value FROM stats ORDER BY key' >$DB.stats.dump
diff -u $DB.stats.dump ${SRC_DIR}/test/t/stats.stats.dump

sqlite3 $DB 'SELECT key, count_nodes, count_ways, count_relations, values_nodes, values_ways, values_relations, cells_nodes, cells_ways FROM keys ORDER BY key' >$DB.keys.dump
diff -u $DB.keys.dump ${SRC_DIR}/test/t/stats.keys.dump

sqlite3 $DB 'SELECT key, value, count_nodes, count_ways, count_relations FROM tags ORDER BY key, value' >$DB.tags.dump
diff -u $DB.tags.dump ${SRC_DIR}/test/t/stats.tags.dump

#-----------------------------------------------------------------------------
