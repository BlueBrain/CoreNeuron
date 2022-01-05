#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh
module load neuron/develop

set -x
TEST_DIR="$1"

# tqperf has extra mod files under modx
if [ "${TEST_DIR}" = "tqperf" ]; then
    cp modx/*.mod mod/
fi

cd $WORKSPACE/${TEST_DIR}
nrnivmodl mod
