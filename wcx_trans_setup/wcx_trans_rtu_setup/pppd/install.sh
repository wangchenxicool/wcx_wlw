#================================================================================
#
#  ____  ____  ____  ____        ____  _____ _____  _     ____ 
# /  __\/  __\/  __\/  _ \      / ___\/  __//__ __\/ \ /\/  __\
# |  \/||  \/||  \/|| | \|_____ |    \|  \    / \  | | |||  \/|
# |  __/|  __/|  __/| |_/|\____\\___ ||  /_   | |  | \_/||  __/
# \_/   \_/   \_/   \____/      \____/\____\  \_/  \____/\_/   
#                                                             
#================================================================================
#!/bin/sh

echo "pppd install..."
install -m 755 -d /usr/sbin/
install -m 755 ./pppd /usr/sbin/
install -m 755 -d /etc/ppp/
install -m 555 ./resolv.conf /etc/ppp/
install -m 755 -d /etc/ppp/peers/
install -m 555 ./gsm /etc/ppp/peers/
install -m 555 ./gsm_chat /etc/ppp/peers/
install -m 555 ./cdma /etc/ppp/peers/
install -m 555 ./cdma_chat /etc/ppp/peers/
install -m 555 ./pap-secrets /etc/ppp/
echo "pppd install finished!"
