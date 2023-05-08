#!/bin/sh
# Access dstuart web page to provide an indication of this nodes IP address
# Set the name for this particular node, no spaces allowed.
MYNAME="DiRPi1_for_tests_in_5209"
curl -s "http://dstuart.physics.ucsb.edu/phoneHome_${MYNAME}" > /dev/null
