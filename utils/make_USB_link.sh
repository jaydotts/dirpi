ECHOUSB=$("$HOME/dirpi/utils/echo_usb.sh")
sudo mkdir -p "$ECHOUSB/NewData/"
rm newdata
ln -s "$ECHOUSB/NewData/" newdata 
