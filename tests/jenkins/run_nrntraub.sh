#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh
source ${JENKINS_DIR:-.}/_test_utils.sh

set -x

CORENRN_TYPE="$1"
MPI_RANKS="$2"
THREADS="$3"

cd $WORKSPACE/nrntraub

# Allocate 1 node
N=1

export CORENEURONLIB=$(pwd)/${CORENRN_TYPE}/libcorenrnmech.so

n=${MPI_RANKS} bb5_run ./x86_64/special -mpi -c mytstop=100 -c use_coreneuron=1 -c nthread=${THREADS} init.hoc

sort -n -k'1,1' -k2 < out.dat | awk 'NR==1 { print; next } { printf "%.3f\t%d\n", $1, $2 }' > out_cn_${CORENRN_TYPE}.sorted

sdiff -s out_cn_${CORENRN_TYPE}.sorted out_nrn.sorted
