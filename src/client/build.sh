##########################################################################
# File Name: build.sh
# Author: baoshan
# mail: baoshanw@foxmail.com
# Created Time: 2020年04月22日 星期三 18时54分02秒
#########################################################################

#!/bin/sh

g++ -std=c++11 -I /media/wbs/work/reactor_server -pthread cclient.cc ../utils.cc -o ../../bin/cclient
g++ -std=c++11 -I /media/wbs/work/reactor_server -pthread sclient.cc ../simple_reactor/dispatcher.cc  ../simple_reactor/socket.cc  ../simple_reactor/conn.cc  ../simple_reactor/conn_manager.cc ../utils.cc -o ../../bin/sclient
