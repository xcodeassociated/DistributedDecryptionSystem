#!/bin/bash
for ((i=0; i <= 100 ; i++))
do
    parameter1="--example_param"
    parameter2="10000"
    printf "\n"
    mpirun --hostfile hosts --npernode 1 ./DDS "$parameter1" "$parameter2"
    printf "\n"
    echo "=============== Bash: [${i}/100] ==============="
done
