#!/bin/bash
# 
# 
# code for primary dirpi - check 
# node dirpis for new data 
#
#
#
node1_user="dirpi01"
node1_ip="128.111.19.148"
node2_user="dirpi09"
node2_ip="128.111.19.45"
USB_DIR=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)

node1_dir="$USB_DIR/dirpi02"
node2_dir="$USB_DIR/dirpi09"

ping_nodes() {
    echo "Pinging nodes"
}

# copies node data to local usb directories. 
# the files are later 
copy_node_data() {
    if [! -d $USB_DIR ]; then echo "USB NOT INSERTED"; return; fi
    echo "Copying data" 
    # prompts node to send location of its usb directory
    # using the onboard dirpi/get_usb.sh command. 
    # it is then stored in a variable.
    node1_usb=$(ssh "$node1_user@$node1_ip" "dirpi/utils/echo_usb.sh")
    node2_usb=$(ssh "$node2_user@$node2_ip" "dirpi/utils/echo_usb.sh")

    if [ -n "$node1_usb" ];then 
        echo "node1 usb located at $node1_usb"
        echo "copying data to run folder $node1_dir" 

        mkdir -p $node1_dir
        scp -r "$node1_user@$node1_ip:$node1_usb/Run*" "$node1_dir/"
        if [ $? -eq 0 ]; then
            # remove the src files
            echo "SCP for node 1 complete, deleting src files."
            ssh "$node1_user@$node1_ip" "rm -rf $node1_usb/Run*"
        else
        echo "[WARNING] SCP for node 1 incomplete. Keeping src files."
        fi
    else
        echo "[WARNING] node1 usb not inserted. Data will not be transferred."
    fi 

    if [ -n "$node2_usb" ]; then 
        echo "node2 usb located at $node2_usb"
        echo "copying data to run folder $node2_dir"

        mkdir -p $node2_dir
        scp -r "$node2_user@$node2_ip:$node2_usb/Run*" "$node2_dir/"
        if [ $? -eq 0 ]; then
            # remove the src files
            ssh "$node2_user@$node2_ip" "rm -rf $node2_usb/Run*"
        else
        echo "[WARNING] SCP for node 2 incomplete. Keeping src files."
        fi
    else 
        echo "[WARNING] node2 usb not inserted. no data will be transferred."
    fi
}

copy_node_data
