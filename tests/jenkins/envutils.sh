#!/bin/bash

# Based on envutils.sh from blueconfigs repo

# NOTE: This file shall be sourced so that important variables are avail to other scripts
[ -n "$ENVUTILS_LOADED" ] && return || readonly ENVUTILS_LOADED=1
_setbk=$-
unset $(set +x; env | awk -F= '/^(PMI|SLURM)_/ {print $1}' | xargs)
set +x
Red='\033[31m'
Green='\033[32m'
Yellow='\033[33m'
Blue='\033[34m'
ColorReset='\033[0m'

log() (
    set +x
    log_type=${2:-INFO}
    echo -e "[${3:-$Blue} $log_type $ColorReset] $1"
)

log_ok() { log "$1\n" ${2:-OK} $Green; }

log_warn() { log "$1" ${2:-WARNING} $Yellow; }

log_error() { log "$1" ${2:-ERROR} $Red; }


# On error abort with a meaningful msg
trap 'log_error "Exit code: ${?}"' ERR


if [[ -z "$ADDITIONAL_ENV_VARS" && -n "$GERRIT_CHANGE_COMMIT_MESSAGE" ]]; then
    ADDITIONAL_ENV_VARS=$(set +x; echo "$GERRIT_CHANGE_COMMIT_MESSAGE" | sed -n "s/^ENV_VARS://p")
fi
log "Checking for override hooks (ADDITIONAL_ENV_VARS or gerrit 'ENV_VARS:')..."
set -$_setbk
[ "$ADDITIONAL_ENV_VARS" ] && eval $ADDITIONAL_ENV_VARS


bb5_run() (
    set +x -e
    # default partition is interactive. during night use production
    hour=`date +%H`
    if [ "$hour" -ge "20" ] || [ "$hour" -lt "8" ]; then export SALLOC_PARTITION="prod"; fi

    N=${N:-1}
    if [ -n "$n" ]; then SALLOC_OPTS="$SALLOC_OPTS -n$n"; fi

    cmd_base="salloc -N$N $SALLOC_OPTS --ntasks-per-node=36 -Aproj16 --hint=compute_bound -Ccpu|nvme --time 1:00:00 srun --pty"
    echo "$cmd_base $@"
    $cmd_base "$@"
)
