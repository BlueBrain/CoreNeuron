#!/usr/bin/bash

set -e
set +x

TEST_DIR="$1"

. /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
export SPACK_INSTALL_PREFIX="${SPACK_INSTALL_PREFIX:-${WORKSPACE}/INSTALL_HOME}"
export MODULEPATH=$SPACK_INSTALL_PREFIX/modules/tcl/linux-rhel7-x86_64:$MODULEPATH

module load intel neuron/develop/python3/parallel

unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')

cd $WORKSPACE/${TEST_DIR}
nrnivmodl mod
