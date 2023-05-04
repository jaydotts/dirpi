# first, build the python environment 

cd "build"

echo "checking for pip..." 

if [ ! $(command -v pip) ]; then 
    echo "INSTALLING PIP"
    install pip
    wait 
else 
    echo "requirement satisfied"
fi 

echo "installed at $(command -v pip)"

pip install -r "requirements.txt"
wait 
cp -r "WiringPi" /
"/WiringPi/build"