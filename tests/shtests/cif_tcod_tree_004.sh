#!/bin/bash

set -ue

cif_tcod_tree=./scripts/cif_tcod_tree
cwd=$(pwd)

BASENAME="`basename $0 .sh`"

TMP_DIR="./tmp-${BASENAME}"

mkdir ${TMP_DIR}

${cif_tcod_tree} --out ${TMP_DIR} <<END || true
data_tcod_cif
loop_
  _tcod_computation_step
  _tcod_computation_command
0
'cat test.cif'
loop_
  _tcod_file_id
  _tcod_file_URI
  _tcod_file_md5sum
  _tcod_file_name
0
file://${cwd}/tests/inputs/tcod-crafted.cif 
ca8955e79b28e812cbd496476f8f6a73
test.cif
END

set -x

tree ${TMP_DIR}
cat ${TMP_DIR}/main.sh || true

set +x

rm -rf ${TMP_DIR}