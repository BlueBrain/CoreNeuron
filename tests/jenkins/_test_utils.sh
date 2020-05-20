#!/bin/bash

bb5_run() (
    set +x -e
    # default partition is interactive. during night use production
    hour=`date +%H`
    weekday=`date +%u`
    if [ "$hour" -ge "19" ] || [ "$hour" -lt "8" ] || [ $weekday -gt 5 ]; then export SALLOC_PARTITION="prod"; fi

    N=${N:-1}
    if [ -n "$n" ]; then
        SALLOC_OPTS="$SALLOC_OPTS --ntasks-per-node=$n"
    else
        SALLOC_OPTS="$SALLOC_OPTS --ntasks-per-node=36"
    fi

    cmd_base="time salloc -N$N $SALLOC_OPTS -Aproj16 --hint=compute_bound -Ccpu|nvme --time 1:00:00 srun dplace "
    echo "$cmd_base $@"
    $cmd_base "$@"
)
