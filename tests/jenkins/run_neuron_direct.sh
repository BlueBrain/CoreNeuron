#!/usr/bin/bash

set -e
set -x

CORENRN_TYPE="$1"

unset MODULEPATH
. /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
export SPACK_INSTALL_PREFIX="${SPACK_INSTALL_PREFIX:-${WORKSPACE}/INSTALL_HOME}"
export MODULEPATH=$SPACK_INSTALL_PREFIX/modules/tcl/linux-rhel7-x86_64:$MODULEPATH

module load hpe-mpi neuron/develop/python3/parallel

export CORENEURONLIB=$WORKSPACE/install_${CORENRN_TYPE}/lib64/libcoreneuron.so

unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')

if [[ "$CORENRN_TYPE" = "GPU" ]]; then
    srun -n 1 --exclusive --account=proj16 --partition=interactive --constraint=volta --gres=gpu:1 --mem 0 -t 00:05:00 nrniv -mpi -python $WORKSPACE/tests/jenkins/neuron_direct.py 2>&1 | tee output.txt
else
    nrniv -python $WORKSPACE/tests/jenkins/neuron_direct.py 2>&1 | tee output.txt
fi

if [[ ! $(grep "Voltage times same: True" output.txt) ]]; then
    exit 1
fi

if [[ ! $(grep "Voltage difference less than 1e-10: True" output.txt) ]]; then
    exit 1
fi
