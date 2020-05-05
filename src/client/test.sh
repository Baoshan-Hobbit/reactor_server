##########################################################################
# File Name: test.sh
# Author: baoshan
# mail: baoshanw@foxmail.com
# Created Time: 2020年05月05日 星期二 12时03分18秒
#########################################################################

#!/bin/sh

# 批量结束进程
#ps -ef | grep sclient | grep -v grep | awk '{print $2}' | xargs kill -15

# 批量产生client,当confirm或收到server的response后发送消息,模拟并发
# 需要注意的是client发送不能太过频繁,可使用sleep(),
# 当不使用sleep()时无论是single thread server还是thread pool server会出现2种
# 莫名其妙的运行结果:
# 当批量启动sclient然后发送 SIGTERM(kill -l 查看编号为15)信号(正常退出,等待
# 进程做完清理工作)批量终止client进程时server出现问题:
# a) 进程突然退出,却没有产生coredump文件,排除系统发送信号导致异常退出
# b) 进程卡住,不能响应后续发起的client请求,cpu持续占用一个核(25%使用率)
# 
# 当在client端引入sleep后该现象消失,猜测原因是:
# client发送太过频繁,且与server在同一台机器,操作系统的调度和进程控制可能
# 哪一块受到了影响
#
# 经验:
# 1) 当进程运行出问题时,首先要怀疑程序运行环境的配置,程序调用的方式,从操作系统
#    等角度排查,而不是一头扎进代码里从代码中找问题
# 2) coredump文件和gdb配合使用可有效定位异常错误(如内存错误,进程收到信号导致的
#    错误等)

for i in `seq 1 1000`
do
  ../../bin/sclient > /dev/null 2>&1 &
done
