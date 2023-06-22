
# code for primary dirpi - check 
# node dirpis for new data 

node1_user=""
node1_ip=""
node2_user=""
node2_ip=""
fetch_usb='readlink -f /dev/disk/by-id/usb-* | while read dev;do mount | grep "$dev\b" | awk "{print $3}"; done)'

ping_nodes() {
    echo "Pinging nodes"
}

copy_data() {
    echo "Copying data" 
    $(ssh "$node1_user@$node1_ip" "$fetch_usb")
    local node1_usb=$?
    $(ssh "$node2_user@$node2_ip" "$fetch_usb")
    local node2_usb=$?
}


