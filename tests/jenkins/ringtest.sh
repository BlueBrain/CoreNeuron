#!/usr/bin/bash

set -e
unset MODULEPATH
. /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
module load unstable
module load hpe-mpi

set -x
CORENRN_TYPE="$1"
export MPI_UNBUFFERED_STDIO=1

if [ "${CORENRN_TYPE}" = "GPU-non-unified" ]  || [ "${CORENRN_TYPE}" = "GPU-unified" ]; then
    unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')
fi

cd $WORKSPACE/build_${CORENRN_TYPE}
echo "Testing ${CORENRN_TYPE}"
ctest --output-on-failure -T test --no-compress-output -E cppcheck_test
