#!/bin/bash
# 
# 
# code for primary dirpi - check 
# node dirpis for new data 
#
#
#
node1_user="dirpi01"
node2_user="dirpi09"
node1_ip="128.111.19.148"
node2_ip="128.111.19.45"
USB_DIR=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)

# sets default count of active nodes 
active_nodes=0
node1_active=0
node2_active=0

# Pings nodes and returns 1 if nodes are active, 0 otherwise 
ping_nodes() {
    echo "Pinging nodes"
    
    if ping -c 1 "$node1_ip" > /dev/null 2>&1; then
        echo "$node1_user active at $node1_ip"
        node1_active=1
        ((active_nodes++))
    else
        echo "[WARNING] node1 ($node1_user) unreachable at $node1_ip"
    fi

    if ping -c 1 "$node2_ip" > /dev/null 2>&1; then
        echo "$node2_user active at $node2_ip"
        node2_active=1
        ((active_nodes++))
    else
        echo "[WARNING] node1 ($node2_user) unreachable at $node2_ip"
    fi
}

setup_nodes(){ 
    echo "Setting up nodes..."
    ping_nodes 

    if [ $node1_active -eq 1 ]; then 
        local node1_id=$(ssh "$node1_user@$node1_ip" "head -n 1 dirpi/metadata/ID.txt")
        if [ -n "$node1_id" ]; then 
            node1_dir="$USB_DIR/dirpi$node1_id"
            echo "Creating node 1 directory at $node1_dir"
            mkdir -p $node1_dir
        fi 
    fi
    if [ $node2_active -eq 1 ]; then 
        local node2_id=$(ssh "$node2_user@$node2_ip" "head -n 1 dirpi/metadata/ID.txt")
        if [ -n "$node2_id" ]; then 
            node2_dir="$USB_DIR/dirpi$node2_id"
            echo "Creating node 2 directory at $node2_dir"
            mkdir -p $node2_dir
        fi 
    fi 

    if [[ "$node1_id" && "$node2_id" ]]; then 
        if [ $node1_id -eq $node2_id ]; then 
            echo "[WARNING] Non-unique node ids in ID.txt"
        fi 
    fi 
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
