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
node3_user="dirpi4"
node1_ip="128.111.19.148"
node2_ip="128.111.19.45"
node3_ip="128.111.19.149"
USB_DIR=$(readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk '{print $3}'; done)
node1_id="$(cat $HOME/dirpi/nodes/node1/ID.txt)"
node2_id="$(cat $HOME/dirpi/nodes/node2/ID.txt)"
node3_id="$(cat $HOME/dirpi/nodes/node3/ID.txt)"

node1_dir="$USB_DIR/dirpi$node1_id"
node2_dir="$USB_DIR/dirpi$node2_id"
node3_dir="$USB_DIR/dirpi$node3_id"

# default globals set in setup_nodes
active_nodes=0
node1_active=0
node2_active=0
node3_active=0

# Pings nodes and returns 1 if nodes are active, 0 otherwise 
ping_nodes() {
    echo "Pinging nodes"
    
    # NODE 1
    if ping -c 1 "$node1_ip" > /dev/null 2>&1; then
        echo "NODE 1: user $node1_user active at $node1_ip"
        node1_active=1
        ((active_nodes++))
        node1_usb=$(ssh "$node1_user@$node1_ip" "dirpi/utils/echo_usb.sh" > /dev/null) 
        if [ ! $? -eq 0 ]; then 
            echo "[WARNING] Node 1 USB not found"
            node1_active=0
        fi
        node1_id=$(ssh "$node1_user@$node1_ip" "cat dirpi/metadata/ID.txt")
        if [ ! -n "$node1_id" ]; then
            echo "[WARNING] Node 1 ID not found"
            node1_active=0
        else 
            echo "NODE 1: Dirpi ID $node1_id"
        fi

    else
        echo "[WARNING] node1 ($node1_user) unreachable at $node1_ip"
    fi
    
    # NODE 2
    if ping -c 1 "$node2_ip" > /dev/null 2>&1; then
        echo "NODE 2: user $node2_user active at $node2_ip"
        node2_active=1
        ((active_nodes++))
        node2_usb=$(ssh "$node2_user@$node2_ip" "dirpi/utils/echo_usb.sh" > /dev/null) 
        if [ ! $? -eq 0 ]; then 
            echo "[WARNING] NODE 2: USB not found"
            node2_active=0
        fi

        node2_id=$(ssh "$node2_user@$node2_ip" "cat dirpi/metadata/ID.txt")
	if [ ! -n "$node2_id" ]; then
	   echo "[WARNING] NODE 2: ID not found"
       node2_active=0
	else 
	   echo "NODE 2: Dirpi ID $node2_id"
        fi

    else
        echo "[WARNING] node 2 ($node2_user) unreachable at $node2_ip"
    fi
    
    # NODE 3
    if ping -c 1 "$node3_ip" > /dev/null 2>&1; then
        echo "NODE 3: user $node3_user active at $node3_ip"
        node3_active=1
        ((active_nodes++))
        node3_usb=$(ssh "$node3_user@$node3_ip" "dirpi/utils/echo_usb.sh" > /dev/null) 
        if [ ! $? -eq 0 ]; then 
            echo "[WARNING] NODE 3: USB not found"
            node3_active=0
        fi

        node3_id=$(ssh "$node3_user@$node3_ip" "cat dirpi/metadata/ID.txt")
	if [ ! -n "$node3_id" ]; then
	   echo "[WARNING] NODE 3: ID not found"
       node3_active=0
	else 
	   echo "NODE 3: Dirpi ID $node3_id"
        fi

    else
        echo "[WARNING] node3 ($node3_user) unreachable at $node3_ip"
    fi
}

setup_nodes(){ 
    echo "Setting up nodes..."
    
    ping_nodes 

    if [ $node1_active -eq 1 ]; then 
        node1_id=$(ssh "$node1_user@$node1_ip" "head -n 1 dirpi/metadata/ID.txt")
        if [ -n "$node1_id" ]; then 
            node1_dir="$USB_DIR/dirpi$node1_id"
            echo "Node 1 directory created at $node1_dir"
            sudo mkdir -p $node1_dir
            echo $node1_id > "$HOME/dirpi/nodes/node1/ID.txt"
        fi 
    fi
    if [ $node2_active -eq 1 ]; then 
        local node2_id=$(ssh "$node2_user@$node2_ip" "head -n 1 dirpi/metadata/ID.txt")
        if [ -n "$node2_id" ]; then 
            node2_dir="$USB_DIR/dirpi$node2_id"
            echo "Node 2 directory created at $node2_dir"
            sudo mkdir -p $node2_dir
            echo $node2_id > "$HOME/dirpi/nodes/node2/ID.txt"
        fi 
    fi 
    if [ $node3_active -eq 1 ]; then 
        local node3_id=$(ssh "$node3_user@$node3_ip" "head -n 1 dirpi/metadata/ID.txt")
        if [ -n "$node3_id" ]; then 
            node3_dir="$USB_DIR/dirpi$node3_id"
            echo "Node 3 directory created at $node3_dir"
            sudo mkdir -p $node3_dir
            echo $node3_id > "$HOME/dirpi/nodes/node3/ID.txt"
        fi 
    fi 
}

push_node_configs(){
    
    local node1_path="$HOME/dirpi/nodes/node1/configs/*"
    local node2_path="$HOME/dirpi/nodes/node2/configs/*"
    local node3_path="$HOME/dirpi/nodes/node3/configs/*"
    
    scp $node1_path "$node1_user@$node1_ip:dirpi/config_templates"
    scp $node2_path "$node2_user@$node2_ip:dirpi/config_templates"
    scp $node3_path "$node3_user@$node3_ip:dirpi/config_templates"
}

# copies node data to local usb directories. 
# the files are later 

monitor_nodes(){
    
    #setup_nodes
    local route=$1
    local NODE=$2
    
    if [ $NODE -eq 1 ]; then 
        while [ 1 ]; do 
        scp "$route:dirpi/plot_buffer.txt" "$HOME/dirpi/nodes/node1_buffer.txt"
        done 
    fi 
    
    if [ $NODE -eq 2 ]; then 
        while [ 1 ]; do 
            scp "$route:dirpi/plot_buffer.txt" "$HOME/dirpi/nodes/node2_buffer.txt"
        done 
    fi 
    
    if [ $NODE -eq 3 ]; then 
        while [ 1 ]; do 
            scp "$route:dirpi/plot_buffer.txt" "$HOME/dirpi/nodes/node3_buffer.txt"
        done 
    fi 
}

copy_node_data() {
    
    # setup node directories
    setup_nodes
    
    # check that USB is inserted before copying
    if [ ! -d $USB_DIR ]; then echo "USB NOT INSERTED"; return; fi
    
    # copy data from each node in series 
    if [ $node1_active -eq 1 ]; then 
        echo "Copying data from node $node1_user@$node1_ip" 
        local node1_usb=$(ssh "$node1_user@$node1_ip" "dirpi/utils/echo_usb.sh")
        
        if [ -n "$node1_usb" ];then 
            echo "node1 usb located at $node1_usb"
            echo "copying data to run folder $node1_dir" 
            mkdir -p $node1_dir
            
            if [ -d "$node1_dir" ]; then
                scp -r "$node1_user@$node1_ip:$node1_usb/Run*" "$node1_dir/"
                if [ $? -eq 0 ]; then
                    # remove the src files
                    echo "SCP for node 1 complete, deleting src files."
                    ssh "$node1_user@$node1_ip" "rm -rf $node1_usb/Run*"
                else
                    echo "[WARNING] SCP for node 1 incomplete. Keeping src files."
                fi
            else 
                echo "[WARNING] Could not access directory $node1_dir. No data copied."
            fi
        
        else 
            echo "[WARNING] node1 usb not inserted. Data will not be transferred."
        fi
    else
        echo "[WARNING] node1 inactive."
    fi 
    
    if [ $node2_active -eq 1 ]; then 
        echo "Copying data from node $node2_user@$node2_ip" 
        local node2_usb=$(ssh "$node2_user@$node2_ip" "dirpi/utils/echo_usb.sh")
        if [ -n "$node2_usb" ];then 
            echo "node2 usb located at $node2_usb"
            echo "copying data to run folder $node2_dir" 
            mkdir -p $node2_dir
            
            if [ -d "$node2_dir" ]; then
                scp -r "$node1_user@$node1_ip:$node1_usb/Run*" "$node2_dir/"
                if [ $? -eq 0 ]; then
                    # remove the src files
                    echo "SCP for node 2 complete, deleting src files."
                    # UN-COMMENT THIS LINE BEFORE OFFICIAL LAUNCH
                    #ssh "$node2_user@$node2_ip" "rm -rf $node2_usb/Run*"
                else
                    echo "[WARNING] SCP for node 2 incomplete. Keeping src files."
                fi
            else 
                echo "[WARNING] Could not access directory $node2_dir. No data copied."
            fi
        
        else 
            echo "[WARNING] node2 usb not inserted. Data will not be transferred."
        fi
    else
        echo "[WARNING] node2 inactive."
    fi 
    
    if [ $node3_active -eq 1 ]; then 
        echo "Copying data from node $node3_user@$node3_ip" 
        local node3_usb=$(ssh "$node3_user@$node3_ip" "dirpi/utils/echo_usb.sh")
        if [ -n "$node3_usb" ];then 
            echo "node3 usb located at $node3_usb"
            echo "copying data to run folder $node3_dir" 
            mkdir -p $node3_dir
            
            if [ -d "$node3_dir" ]; then
                scp -r "$node3_user@$node3_ip:$node3_usb/Run*" "$node3_dir/"
                if [ $? -eq 0 ]; then
                    # remove the src files
                    echo "SCP for node 3 complete, deleting src files."
                    # UN-COMMENT THIS LINE BEFORE OFFICIAL LAUNCH
                    #ssh "$node3_user@$node3_ip" "rm -rf $node3_usb/Run*"
                else
                    echo "[WARNING] SCP for node 3 incomplete. Keeping src files."
                fi
            else 
                echo "[WARNING] Could not access directory $node3_dir. No data copied."
            fi
        
        else 
            echo "[WARNING] node3 usb not inserted. Data will not be transferred."
        fi
    else
        echo "[WARNING] node3 inactive."
    fi
}
