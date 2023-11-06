#!/bin/bash
#
# Script to automagically install nvi for GNU/Linux and Mac OS systems
#
# Copyright (c) 2023 by Matt Carlotta
#
# Bugs:
#     - Bug reports can be filed at: https://github.com/mattcarlotta/nvi/issues
#        Please provide clear steps to reproduce the bug and the output of the
#        script. Thank you!

tput_bin=$(which tput)
bold=$($tput_bin bold)
underline=$($tput_bin smul)
nounderline=$($tput_bin rmul)
normal=$($tput_bin sgr0)
red=$($tput_bin setaf 1)
green=$($tput_bin setaf 2)
yellow=$($tput_bin setaf 3)
blue=$($tput_bin setaf 4)
# magenta=$($tput_bin setaf 5)
# cyan=$($tput_bin setaf 6)

cmake_bin=$(which cmake)
make_bin=$(which make)
curl_bin=$(which curl-config)

log_error() {
    echo -e "\n❌ ${bold}${red}ERROR: $1 ${normal}"
    exit 1
}

log_warning_with_icon() {
    echo -e "⚠️  ${bold}${yellow}$1 ${normal}"
}

log_warning() {
    echo -e "${bold}${yellow}$1 ${normal}"
}

log_success() {
    echo -e "✅ ${bold}${green}$1 ${normal}"
}

install_bin() {
    log_warning_with_icon "In order to install nvi to your system, this script requires ${blue}${underline}ROOT${nounderline}${yellow} level privileges."
    log_warning "You can skip this step by pressing ctrl + c and manually installing the nvi binary to your system."
    log_warning "Otherwise, enter your password below to install the nvi binary to ${blue}${underline}/usr/local/bin${nounderline}${yellow}."

    sudo make install > /dev/null 2>&1
    if [[ $? -ne 0 ]];
    then
        log_error "Failed to install the nvi binary to your system."
    else
        log_success "Installed the nvi binary to your system: ${blue}${underline}$(which nvi)${nounderline}${green}!"
    fi
}

generate_cmake_cache() {
    local cmake_result=$($cmake_bin . 2>&1)
    if [[ "$cmake_result" =~ "CMake Error" ]] || [[ "$cmake_result" =~ "CMake Warning" ]];
    then
        log_error "Failed to generate cmake cache. Ensure you're within the root level of the nvi project directory.\n$cmake_result"
    else
        log_success "Generated cmake cache!"
    fi
}

compile_bin() {
    generate_cmake_cache

    local cmake_build_result=$($cmake_bin --build . -j $1 2>&1 1>/dev/null)
    if [[ ! -z "$cmake_build_result" ]];
    then
        log_error "Failed to compile the nvi binary.\n$cmake_build_result"
    else
        log_success "Compiled the nvi binary!"
    fi
}

compile_for_mac() {
    local sysctl_bin=$(which sysctl)
    local cpu_core_count=$($sysctl_bin -n hw.ncpu)
    compile_bin $cpu_core_count
}

compile_for_gnu_linux() {
    local cpu_core_count=$(nproc --all)
    compile_bin $cpu_core_count
}

check_system_requirements() {
    local install_message="doesn't appear to be installed on the system. You must install it before using this script. Optionally you can manually build and install nvi by reading the Custom Installations section in the GitHub wiki."
    if [ -z "$cmake_bin" ];
    then
        log_error "cmake $install_message"
    fi

    if [ -z "$make_bin" ];
    then
        log_error "make $install_message"
    fi

    if [ -z "$curl_bin" ];
    then
        log_error "curl doesn't appear to be installed on the system. You must install it before compiling the source."
    fi

}

main() {
    check_system_requirements

    case "$OSTYPE" in
        darwin*)  compile_for_mac ;; 
        linux*)   compile_for_gnu_linux ;;
        *)        log_error "The following system is not supported by this script: $OSTYPE" ;;
    esac

    install_bin
}

trap '{ exit 1; }' INT
main
