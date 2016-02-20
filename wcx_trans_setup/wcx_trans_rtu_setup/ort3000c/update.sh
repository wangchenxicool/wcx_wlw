#================================================================================
#
#                 _                                 _           _          
#   ___    _ __  | |_           _   _   _ __     __| |   __ _  | |_    ___ 
#  / _ \  | '__| | __|  _____  | | | | | '_ \   / _` |  / _` | | __|  / _ \
# | (_) | | |    | |_  |_____| | |_| | | |_) | | (_| | | (_| | | |_  |  __/
#  \___/  |_|     \__|          \__,_| | .__/   \__,_|  \__,_|  \__|  \___|
#                                      |_|                                 
#
#================================================================================
#!/bin/sh

ORT3000_PATH=$1
PATH_UPDATE=$ORT3000_PATH/update
PATH_CODE=$ORT3000_PATH/code
PATH_DATA=$ORT3000_PATH/data

# create folder                                                                 
mkdir -p $PATH_CODE/driver/
mkdir -p $PATH_CODE/collect/bin/
mkdir -p $PATH_CODE/collect/database/fast_tag/
mkdir -p $PATH_CODE/gsm_online/
mkdir -p $PATH_CODE/vpn_online/
mkdir -p $PATH_DATA/wcx/log/

# update lib                                                                    
install -m 755 -d /usr/lib/
tar xzvf $PATH_UPDATE/ort3000c_lib.tgz -C /usr/lib/                             
                                                                                
# update driver                                                                 
cp -rf $PATH_UPDATE/driver/* $PATH_CODE/driver/                                 
                                                   
# update vpn_online                                
cp -rf $PATH_UPDATE/vpn_online/* $PATH_CODE/vpn_online/
                                                       
# update collect                                       
cp -rf $PATH_UPDATE/collect/bin/* $PATH_CODE/collect/bin/
