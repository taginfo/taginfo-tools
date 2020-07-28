#!/bin/sh
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

DATA=${SRC_DIR}/test/data.opl
DB=osmstats.db

rm -f $DB
${BIN_DIR}/src/osmstats $DATA $DB

sqlite3 $DB 'SELECT key, value FROM stats ORDER BY key' >$DB.stats.dump

diff -u $DB.stats.dump ${SRC_DIR}/test/t/stats.stats.dump

#-----------------------------------------------------------------------------
