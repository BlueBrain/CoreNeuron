#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh
source ${JENKINS_DIR:-.}/_test_utils.sh

set -x
TEST_DIR="$1"
TEST="$2"
MPI_RANKS="$3"
THREADS="$4"

cd $WORKSPACE/${TEST_DIR}

if [ "${TEST_DIR}" = "testcorenrn" ]; then
    mkdir test${TEST}dat
    mkdir ${TEST}
    mpirun -n ${MPI_RANKS} ./x86_64/special -mpi -c sim_time=100 test${TEST}.hoc
    cat out${TEST}.dat | sort -k 1n,1n -k 2n,2n > ${TEST}/out_nrn_${TEST}.spk
    rm out${TEST}.dat
elif [ "${TEST_DIR}" = "ringtest" ]; then
    mkdir ${TEST}
    mpirun -n 6 ./x86_64/special ringtest.py -mpi
    cat coredat/spk6.std | sort -k 1n,1n -k 2n,2n > ${TEST}/out_nrn_${TEST}.spk
elif [ "${TEST_DIR}" = "tqperf" ]; then
    mkdir ${TEST}
    mpirun -n ${MPI_RANKS} ./x86_64/special -c tstop=50 run.hoc -mpi
    cat spk000.dat | sort -k 1n,1n -k 2n,2n > ${TEST}/out_nrn_${TEST}.spk
elif [ "${TEST_DIR}" = "nrntraub" ]; then
    # Allocate 1 node
    N=1
    n=${MPI_RANKS} bb5_run ./x86_64/special -mpi -c mytstop=100 -c use_coreneuron=0 -c nthread=${THREADS} init.hoc
    sort -n -k'1,1' -k2 < out${MPI_RANKS}.dat | awk 'NR==1 { print; next } { printf "%.3f\t%d\n", $1, $2 }' > out_nrn.sorted
else
    echo "Not a valid TEST"
    exit 1
fi
