#!/bin/sh
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

DATA=${SRC_DIR}/test/unicode.opl
DB=unicode.db

rm -f $DB
sqlite3 $DB <${SRC_DIR}/test/init.sql
sqlite3 $DB <${SRC_DIR}/test/pre.sql
${BIN_DIR}/src/taginfo-stats $DATA $DB

${BIN_DIR}/src/taginfo-unicode $DB

sqlite3 $DB 'SELECT * FROM key_characters ORDER BY key, num;' >unicode.dump

diff -u unicode.dump - <<'EOF'
a❤b|0|a|U+0061|1|Ll|0|LATIN SMALL LETTER A
a❤b|1|❤|U+2764|56|So|10|HEAVY BLACK HEART
a❤b|2|b|U+0062|1|Ll|0|LATIN SMALL LETTER B
a⧘b|0|a|U+0061|1|Ll|0|LATIN SMALL LETTER A
a⧘b|1|⧘|U+29d8|105|Ps|10|LEFT WIGGLY FENCE
a⧘b|2|b|U+0062|1|Ll|0|LATIN SMALL LETTER B
EOF

#-----------------------------------------------------------------------------
