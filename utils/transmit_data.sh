#!/bin/bash
#
# Trasmits data from node pi to an ip address supplied when calling 
# the script. 
#
#

user=$1
source_ip=$2
path=$3

if [[ ! -n "$requested_ip" || ! -n "$requested_path" ]];then 
    exit 2
fi 

USB_DIR=$(source $HOME/dirpi/utils/echo_usb.sh)
scp -r "$USB_DIR/Run*" "$source_