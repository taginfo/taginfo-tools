#!/bin/sh
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

sizeof=$(${BIN_DIR}/src/taginfo-sizes | cut -c1-6 | uniq)

test $sizeof = "sizeof"

#-----------------------------------------------------------------------------
