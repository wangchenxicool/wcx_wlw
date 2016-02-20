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

echo "ssh install..."
install -m 755 -d /lib
install -m 755 -d /bin
install -m 755 -d /etc
install -m 755 -d /etc/ssh
install -m 755 -d /libexec
install -m 755 -d /sbin
install -m 755 -d /share

install -m 755 ./lib/*  /lib/
install -m 755 ./bin/*  /bin/
install -m 755 ./etc/ssh/*  /etc/ssh/
install -m 755 ./libexec/*  /libexec/
install -m 755 ./sbin/*  /sbin/
install -m 755 ./share/*  /share/

cd /share/
tar xzvf man.tgz
rm man.tgz
cd /etc/ssh/
tar xzvf ssh_config.tgz
rm ssh_config.tgz

echo "sshd:x:110:65534::/var/run/sshd:/usr/sbin/nologin" >> /etc/passwd
echo "sshd:!:14069:0:99999:7:::" >> /etc/shadow
#echo "#run ssh" >> /etc/init.d/rcS
#echo "/sbin/sshd" >> /etc/init.d/rcS

echo "ssh install finished!"
