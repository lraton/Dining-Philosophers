#!/bin/bash

if [[ $# != 4 ]];
then
    echo "Devi mettere 4 parametri: Numero filosofi, check deadlock(1 o 0), algoritmo no deadlock (1 o 0), check starvation (1 o 0)"
else
    if [[ $2 != 0 ]];
    then
        if [[ $2 != 1 ]];
        then
            echo "La prima flag non è valida, deve essere 1 o 0"
            exit 1
        fi
    fi
    if [[ $3 != 0 ]];
    then
        if [[ $3 != 1 ]];
        then
            echo "La seconda flag non è valida, deve essere 1 o 0"
            exit 1
        fi
    fi
    if [[ $4 != 0 ]];
    then
        if [[ $4 != 1 ]];
        then
            echo "La terza flag non è valida, deve essere 1 o 0"
            exit 1
        fi
    fi
    gcc cenadeifilosofi.c -o esame -pthread
    chmod 777 esame
    ./esame $1 $2 $3 $4
fi
exit 1
