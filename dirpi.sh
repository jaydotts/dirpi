
cd /home/dirpi0/ucsb-digitizer/

kill_recurse() {
    cpids=`pgrep -P $1|xargs`
    for cpid in $cpids;
    do
        kill_recurse $cpid
    done
    echo "killing $1"
    kill -9 $1
}

case $1 in 
    start)
        make compiler

        if [ ! -d "DiRPi0_DATA" ]; then 
            mkdir "DiRPi0_DATA"
            wait
        fi
        touch "pid.txt"
        make -j2 & pid=$!
        echo "pid=$pid" > pid.txt

        sleep 100 
        sudo pkill -P $pid
        ;;

    stop) 
        . "pid.txt"
        sudo pkill -P $pid
        ;;

    restart)
        . "pid.txt"
        sudo pkill -P $pid
        wait
        make compiler

        if [ ! -d "DiRPi0_DATA" ]; then 
            mkdir "DiRPi0_DATA"
            wait
        fi
        touch "pid.txt"
        make -j2 & pid=$!
        echo "pid=$pid" > pid.txt
        ;;

    *) 
        echo "Nothing to do"
esac