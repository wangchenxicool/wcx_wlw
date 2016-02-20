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

ORT3000_PATH=/ort3000

PATH_UPDATE=$ORT3000_PATH/update
PATH_CODE=$ORT3000_PATH/code
PATH_DATA=$ORT3000_PATH/data

LOCAL_PATH=$(pwd)

# create ort3000 folder 
install -m 755 -d $PATH_UPDATE
install -m 755 -d $PATH_CODE
install -m 755 -d $PATH_DATA
install -m 755 -d $PATH_DATA/wyx
install -m 755 -d $PATH_DATA/wyx/history
install -m 755 -d $PATH_DATA/wyx/alarm
install -m 755 -d $PATH_DATA/wcx
install -m 755 -d $PATH_DATA/wcx/log

# mount partition to ort3000 folder
mount -t yaffs2 /dev/mtdblock4 $PATH_UPDATE
mount -t yaffs2 /dev/mtdblock2 $PATH_CODE
mount -t yaffs2 /dev/mtdblock5 $PATH_DATA

# install ort3000
cd $LOCAL_PATH/ort3000c
./install.sh $ORT3000_PATH

# install pppd
cd $LOCAL_PATH/pppd
./install.sh

# install vpn_client
cd $LOCAL_PATH/vpn_client
./install.sh

# install ssh
cd $LOCAL_PATH/ssh
./install.sh
