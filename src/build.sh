##########################################################################
# File Name: ../worker_reactor/build.sh
# Author: baoshan
# mail: baoshanw@foxmail.com
# Created Time: 2020年05月03日 星期日 16时15分44秒
#########################################################################

#!/bin/sh

g++ -g -std=c++11 -I /media/wbs/work/reactor_server -pthread test.cc utils.cc worker_reactor/thread_pool.cc -o ../bin/test
