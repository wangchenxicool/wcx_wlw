#===============================================================
#
#  _  _  ____  _  _       ___  ____  ____  __  __  ____ 
# ( \/ )(  _ \( \( ) ___ / __)( ___)(_  _)(  )(  )(  _ \
#  \  /  )___/ )  ( (___)\__ \ )__)   )(   )(__)(  )___/
#   \/  (__)  (_)\_)     (___/(____) (__) (______)(__)  
#
#===============================================================
#!/bin/sh

echo "vpn-client install ..."
install -m 755 -d /usr/sbin/
install -m 755 ./pptp /usr/sbin/
install -m 755 -d /etc/ppp/
install -m 755 ./options.pptp /etc/ppp/
install -m 755 ./chap-secrets /etc/ppp/
install -m 755 -d /etc/ppp/peers/
install -m 755 ./vpn /etc/ppp/peers/
echo "vpn-client install finished!"
