#================================================================================
#
#                 _                          _                   
#   ___    _ __  | |_           ___    ___  | |_   _   _   _ __  
#  / _ \  | '__| | __|  _____  / __|  / _ \ | __| | | | | | '_ \ 
# | (_) | | |    | |_  |_____| \__ \ |  __/ | |_  | |_| | | |_) |
#  \___/  |_|     \__|         |___/  \___|  \__|  \__,_| | .__/ 
#                                                         |_|    
#
#================================================================================
#!/bin/sh

ORT3000_PATH=$1
PATH_UPDATE=$ORT3000_PATH/update
PATH_CODE=$ORT3000_PATH/code
PATH_DATA=$ORT3000_PATH/data

# create folder
install -m 755 -d /usr/lib/
install -m 755 -d /etc/init.d/

# install crond
install -m 755 -d /var/spool/cron/crontabs
install -m 755 ./root /var/spool/cron/crontabs/
install -m 755 ./root /usr/bin/

# install update
echo "update install..."
tar xzvf ./ort3000c.tgz -C $PATH_UPDATE
install -m 755 ./ort3000c_lib.tgz $PATH_UPDATE
install -m 755 ./update.sh  $PATH_UPDATE
rm -rf $PATH_UPDATE/driver_for_linux*

# install ort3000
echo "ort3000c install..."
tar xzvf ./ort3000c.tgz -C $PATH_CODE
tar xzvf ./ort3000c_lib.tgz -C /usr/lib/

# install conf
install -m 777 ./rcS /etc/init.d/
install -m 777 ./collect.conf /etc/
install -m 777 ./resolv.conf /etc/
install -m 555 ./ifconfig-eth0 /etc/init.d/
install -m 755 ./chat /usr/sbin/
install -m 755 ./eth0-setting /etc/
install -m 755 ./iec104_master.conf /etc/
install -m 755 ./passwd /etc/
install -m 755 ./ort3000.conf /etc/
install -m 777 ./modbus_slave_no_kill.bin /usr/sbin/
install -m 777 ./demo.bin /usr/bin/
#install -m 755 ./up /usr/sbin/
rm -rf $PATH_CODE/driver_for_linux*
