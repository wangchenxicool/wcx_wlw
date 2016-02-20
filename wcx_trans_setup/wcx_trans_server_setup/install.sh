#================================================================================
#                                   _                                        
# __      __   ___  __  __         | |_   _ __    __ _   _ __    ___         
# \ \ /\ / /  / __| \ \/ /         | __| | '__|  / _` | | '_ \  / __|        
#  \ V  V /  | (__   >  <          | |_  | |    | (_| | | | | | \__ \  _   _ 
#   \_/\_/    \___| /_/\_\  _____   \__| |_|     \__,_| |_| |_| |___/ (_) (_)
#                          |_____|                                           
#================================================================================
#!/bin/sh

WCX_TRANS_PAHT=$1

# create folder

# install crond

# install lrzsz
#echo "install lrzsz.."
#tar xzvf ./lrzsz.tgz -C /usr/bin/

# install phlinux(花生壳客户端)
echo "install phlinux.."
tar xzvf ./phlinux.tgz -C /usr/local/

# install conf
install -m 777 ./trans_sysdb.db /etc/trans_sysdb.db
install -m 777 ./interfaces /etc/network/
install -m 777 ./rc.local /etc/init.d/
install -m 777 ./libiop.so /usr/lib/

# install wcx_trans..
echo "install wcx_trans.."
install -m 777 ./demo /usr/bin/
tar xzvf ./wcx_trans.tgz -C /home
