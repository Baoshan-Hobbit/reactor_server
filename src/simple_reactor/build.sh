##########################################################################
# File Name: build.sh
# Author: baoshan
# mail: baoshanw@foxmail.com
# Created Time: 2020年04月22日 星期三 18时54分02秒
#########################################################################

#!/bin/sh

g++ -std=c++11 -I /media/wbs/work/reactor_server server.cc dispatcher.cc socket.cc conn.cc conn_manager.cc ../utils.cc -o ../../bin/single_thread_server
#g++ -std=c++11 -I /media/wbs/work/reactor_server -pthread cclient.cc ../utils.cc -o ../../bin/cclient
#g++ -std=c++11 -I /media/wbs/work/reactor_server -pthread sclient.cc dispatcher.cc socket.cc conn.cc conn_manager.cc ../utils.cc -o ../../bin/sclient
