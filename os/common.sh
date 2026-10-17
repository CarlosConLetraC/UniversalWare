#!/usr/bin/env bash

function prettyprint() {
    local level=$1
    shift
    local packed=("$@")

    case $level in
        0)
            printf "\e[0;36m[INFO]:\e[0m %s\n" "${packed[*]}"
        ;;
        1)
            printf "\e[0;33m[WARN]:\e[0m %s\n" "${packed[*]}"
        ;;
        2|*)
            printf "\e[0;31m[FAIL]:\e[0m %s\n" "${packed[*]}"
        ;;
    esac
}