#!/bin/bash

# check for missing depend, ask to install
install_dependency() {
    local package="$1"
    local answer

    read -r -p "$package is missing. Install it? [y/N]: " answer

    case "$answer" in
        [yY]|[yY][eE][sS])
            sudo apt-get update || exit 1
            sudo apt-get install -y "$package" || exit 1
            ;;
        *)
            echo "Installation declined. Exiting."
            exit 1
            ;;
    esac
}

# check for missing g++ compiler
if ! command -v g++ >/dev/null 2>&1; then
    install_dependency g++
fi

# check for libcurl development files.
if ! dpkg-query -W -f='${Status}\n' libcurl4-openssl-dev 2>/dev/null \
    | grep -qx 'install ok installed'; then
    install_dependency libcurl4-openssl-dev
fi

# compile the program.
g++ -std=c++17 -Wall -Wextra tinyping.cpp \
    -I./cpp-icmplib -o tinyping -lcurl -pthread || exit 1

echo "Compilation successful."