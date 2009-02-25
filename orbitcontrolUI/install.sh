#! /bin/bash

FILELIST="bin/RTEMS-pc386/*.obj db dbd orbitcontrolUI.cmd envPaths"
USAGE="Usage: $0 install-directory-name"

install( ) {

#    if [ $USER != 'epics' ]; then
#        echo "Error: this script must be run as user: epics"
#        exit 1
#    fi
    test -d $1 || (echo "$1 doesn't exist"; exit 1)
    echo "Installing $FILELIST to $1"
    cp -ruf $FILELIST $1
    echo "Done!"
    exit 0
}

set -ex

if [ $# != 1 ]; then
    echo $USAGE;
    exit 1;
fi

install $1
