#!/usr/bin/bash

set -x
set -e

export SPACK_INSTALL_PREFIX="${SPACK_INSTALL_PREFIX:-${WORKSPACE}/INSTALL_HOME}"
source $WORKSPACE/BUILD_HOME/spack/share/spack/setup-env.sh
source /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
export PATH=$WORKSPACE/BUILD_HOME/spack/bin:/usr/bin:$PATH
export MODULEPATH=$SPACK_INSTALL_PREFIX/modules/tcl/$(spack arch):$MODULEPATH
export CORENEURON_REPO=https://github.com/BlueBrain/CoreNeuron.git
export CORENEURON_BRANCH=${CORENEURON_BRANCH:-"master"}
export CORENEURON_ROOT=${BUILD_HOME}/CoreNeuron

unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')

git clone $CORENEURON_REPO $CORENEURON_ROOT --depth 1 -b $CORENEURON_BRANCH
spack diy neuron@develop
module av neuron
