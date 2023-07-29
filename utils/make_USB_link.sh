ECHOUSB=$("utils/echo_usb.sh")
mkdir -p "$ECHOUSB/NewData/"
rm newdata
ln -s "$ECHOUSB/NewData/" newdata 
