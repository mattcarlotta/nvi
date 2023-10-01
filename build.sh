#!/bin/bash
#
# Script to build nvi for GNU/Linux because I'm too lazy to
# remember cmake compile flags and to compile using all cpu cores
#

tput_bin=$(which tput)
bold=$($tput_bin bold)
normal=$($tput_bin sgr0)
red=$($tput_bin setaf 1)
green=$($tput_bin setaf 2)

cmake_bin=$(which cmake)
make_bin=$(which make)
cpu_core_count=$(grep -m 1 'cpu cores' /proc/cpuinfo | sed 's/.*: //')
use_localhost_api_flag=-DUSE_LOCALHOST_API=ON
compile_tests_flag=-DCOMPILE_TESTS=ON

log_error() {
    echo -e "\n❌ ${bold}${red}ERROR: $1 ${normal}"
    exit 1
}

log_success() {
    echo -e "✅ ${bold}${green}$1 ${normal}"
}

compile_bin() {
    local flags

    if [[ "$1" == "local" ]];
    then
        flags="${use_localhost_api_flag}"
    elif [[ "$1" == "test" ]];
    then
        flags="${compile_tests_flag}"
    else
        flags="${use_localhost_api_flag} ${compile_tests_flag}"
    fi

    local cmake_result=$($cmake_bin ${flags} . 2>&1)
    if [[ "$cmake_result" =~ "CMake Error" ]] || [[ "$cmake_result" =~ "CMake Warning" ]];
    then
        log_error "Failed to generate cmake cache. Ensure you're within the root level of the nvi project directory.\n$cmake_result"
    else
        log_success "Generated cmake cache!"
    fi

    local cmake_build_result=$($cmake_bin --build . -j $cpu_core_count 2>&1 1>/dev/null)
    if [[ ! -z "$cmake_build_result" ]];
    then
        log_error "Failed to compile the nvi binary.\n$cmake_build_result"
    else
        log_success "Compiled the nvi binary using ${flags} flags!"
    fi
}

main() {
    case "$1" in
        local*)  compile_bin "local" ;;
        test*)   compile_bin "test" ;;
        *)       compile_bin     ;;
    esac
}

trap '{ exit 1; }' INT
main $1
