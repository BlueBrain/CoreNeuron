#!/usr/bin/bash

set -e

module load hpe-mpi
TEST_DIR="$1"
CORENRN_TYPE="$2"
TEST="$3"
MPI_RANKS="$4"

cd $WORKSPACE/${TEST_DIR}

if [ "${TEST_DIR}" = "testcorenrn" ] || [ "${CORENRN_TYPE}" = "AoS" ]; then
    export OMP_NUM_THREADS=1
else
    export OMP_NUM_THREADS=2
fi

if [ "${TEST}" = "patstim" ]; then
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 input --pattern patstim.spk -d test${TEST}dat output -o ${TEST}
elif [ "${TEST}" = "ringtest" ]; then
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 input -d coredat output -o ${TEST}
else
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 input -d test${TEST}dat output -o ${TEST}
fi

cat ${TEST}/out.dat > ${TEST}/out_cn_${TEST}.spk
rm ${TEST}/out.dat
diff -w -q ${TEST}/out_nrn_${TEST}.spk ${TEST}/out_cn_${TEST}.spk
