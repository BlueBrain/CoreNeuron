#!/usr/bin/env bash

set -e

export SPACK_INSTALL_PREFIX="${SPACK_INSTALL_PREFIX:-${WORKSPACE}/INSTALL_HOME}"
BUILD_HOME="${WORKSPACE}/BUILD_HOME"
export SPACK_ROOT="${BUILD_HOME}/spack"
export PATH=$SPACK_ROOT/bin:/usr/bin:\$PATH
source /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
export MODULEPATH=$SPACK_INSTALL_PREFIX/modules/tcl/linux-rhel7-x86_64:$MODULEPATH
unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')

spack install neuron@develop
module av neuron