
cd /home/dirpi4/digi_refactor/
output_folder="OUTPUTS"

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

        if [ ! -d "$output_folder" ]; then 
            mkdir "$output_folder"
            wait
        fi
        touch "pid.txt"
        make -j4 & pid=$!
        echo "pid=$pid" > pid.txt
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

        if [ ! -d "$output_folder" ]; then 
            mkdir "$output_folder"
            wait
        fi
        touch "pid.txt"
        make -j4 & pid=$!
        echo "pid=$pid" > pid.txt
        ;;

    *) 
        echo "Nothing to do"
esac
