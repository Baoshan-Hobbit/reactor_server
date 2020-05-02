##########################################################################
# File Name: tool.sh
# Author: baoshan
# mail: baoshanw@foxmail.com
# Created Time: 2020年04月21日 星期二 19时00分45秒
#########################################################################

#!/bin/sh

sed -i "s/server_ip_/server_ip_/g" `ls`
sed -i "s/server_port_/server_port_/g" `ls`
sed -i "s/client_ip_/client_ip_/g" `ls`
sed -i "s/client_port_/client_port_/g" `ls`
