#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh

set -x
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
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 --pattern patstim.spk -d testpatstimdat -o ${TEST}
elif [ "${TEST}" = "patstim_save_restore" ]; then
    # split patternstim file into two parts : total 2000 events, split at line no 1000 (i.e. 50 msec)
    echo 1000 > patstim.1.spk
    sed -n 2,1001p patstim.spk  >> patstim.1.spk
    echo 1000 > patstim.2.spk
    sed -n 1002,2001p patstim.spk  >> patstim.2.spk

    # run test with checkpoint : part 1
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 50 --pattern patstim.1.spk -d testpatstimdat -o ${TEST} --checkpoint checkpoint
    mv out.dat out.1.dat

    # run test with restore : part 2
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 --pattern patstim.2.spk -d testpatstimdat -o ${TEST} --restore checkpoint
    mv out.dat out.2.dat

    # run additional restore by providing full patternstim : part 3
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 --pattern patstim.spk -d testpatstimdat -o ${TEST} --restore checkpoint
    mv out.dat out.3.dat

    # part 2 and part 3 should be same (part 3 ignore extra events)
    diff -w out.2.dat out.3.dat

    # combine spikes
    cat out.1.dat out.2.dat > out.dat
elif [ "${TEST}" = "ringtest" ]; then
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 -d coredat -o ${TEST}
elif [ "${TEST}" = "tqperf" ]; then
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 50 -d coredat --multisend -o ${TEST}
else
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 -d test${TEST}dat -o ${TEST}
fi

mv ${TEST}/out.dat ${TEST}/out_cn_${TEST}.spk
diff -w ${TEST}/out_nrn_${TEST}.spk ${TEST}/out_cn_${TEST}.spk
