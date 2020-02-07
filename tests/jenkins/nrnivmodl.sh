#!/usr/bin/bash

set -e
source ${JENKINS_ROOT:-.}/_env_setup.sh

set -x
TEST_DIR="$1"

cd $WORKSPACE/${TEST_DIR}
nrnivmodl mod
