#!/usr/bin/bash

set -e
set +x

TEST_DIR="$1"

. /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
module load intel
module load `module av neuron 2>&1 | grep -o -m 1 'neuron.*/parallel' | awk -F' ' '{print $1}'`

unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')

cd $WORKSPACE/${TEST_DIR}
nrnivmodl mod
