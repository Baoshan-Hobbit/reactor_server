/*************************************************************************
  > File Name: dispatcher.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月22日 星期三 15时12分42秒
 ************************************************************************/

#include "src/simple_reactor/dispatcher.h"

#include <sys/epoll.h>
#include <stdio.h>
#include <memory>

#include "src/simple_reactor/socket.h"
#include "src/types.h"
#include "src/utils.h"

Dispatcher& Dispatcher::GetInstance() {
  int event_num = 1024;
  uint32_t wait_timeout = 10;
  static Dispatcher dispatcher(event_num, wait_timeout);
  return dispatcher;
}

Dispatcher::Dispatcher(int event_num, uint32_t wait_timeout) : 
                      event_num_(event_num), 
                      wait_timeout_(wait_timeout), 
                      is_running_(false) { 
  epfd_ = epoll_create(event_num_); 
  if (epfd_ == EPOLL_ERROR)
    printf("epoll init failed\n");
}

// 单reactor单线程模式,accept, send/write, compute均在一个线程中完成,
// 不同socket就绪后按顺序处理,不同socket的send/write-compute独自运行
//
// 改进: 加入工作者线程池
// 目的: 将compute单独拿出来,众多socket的请求统一交给工作者线程池处理,
// 主线程只负责socket读写,使得计算和I/O可以并行处理.加快响应时间
// 方法: 
// a) dispatcher增加thread_pool成员, response队列
// b) socket OnRead()时将request连同socket_fd打包成任务进入thread_pool的任务队列
// c) 任务处理完毕后放进response队列
// d) dispatcher增加一个loop()函数,在主线程中运行,周期性的从response队列取响应,
//    根据socket_fd和conn_manager找到对应的conn,然后放进conn的out_buffer
// e) dispatcher中的epoll扫描就绪的socket,socket OnWrite时根据回调函数自动从
//    out_buffer_中取数据发送
void Dispatcher::Start() {
  epoll_event events[event_num_];
  if (is_running_) 
    return;
  is_running_ = true;
  // 这里是死循环,即使在Stop()中改变is_running为false,也无法跳出循环而退出
  // 因此Stop()无意义,结束进程即可
  // is_running的真正用途是避免错误的多次调用epoll_wait,不能epoll_wait两次
  while (is_running_) {
    int ready_fd_num = epoll_wait(epfd_, events, event_num_, wait_timeout_);
    for (int i=0; i<ready_fd_num; ++i) {
      SOCKET socket_fd = events[i].data.fd;
      // epoll event只能使用fd,因此需要存储fd->Socket*,而非只是Socket*
      std::shared_ptr<Socket> socket = FindSocket(socket_fd);
      if (!socket)
        continue;
      if (events[i].events & EPOLLIN)
        socket->OnRead();
      if (events[i].events & EPOLLOUT)
        socket->OnWrite();
      //if (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP))
      if (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP))
       //这里printf引起socket不正常close
        //printf("socket onclose.\n");
        socket->OnClose();
    }
  }
}

void Dispatcher::AddEvent(SOCKET socket_fd) {
  epoll_event ev;
  // 注意这里要加入EPOLLET
  ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLPRI | EPOLLERR | EPOLLHUP;
  //ev.events = EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP; // LT模式
  // EPOLLRDHUP不支持
  //ev.events = EPOLLIN | EPOLLOUT | EPOLLET |EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
  ev.data.fd = socket_fd;
  int ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, socket_fd, &ev);
  if (ret == EPOLL_ERROR)
    printf("add socket_fd %d to epoll failed, errer code=%d\n", socket_fd, 
        GetErrorCode());
}

void Dispatcher::RemoveEvent(SOCKET socket_fd) {
  int ret = epoll_ctl(epfd_, EPOLL_CTL_DEL, socket_fd, nullptr);
  if (ret == EPOLL_ERROR)
    printf("remove socket_fd %d to epoll failed, error code=%d\n", socket_fd, 
        GetErrorCode());
}

void Dispatcher::RegisterSocket(SOCKET socket_fd, std::shared_ptr<Socket> socket) {
  socket_map_.insert(std::make_pair(socket_fd, socket)); 
}
void Dispatcher::UnregisterSocket(SOCKET socket_fd) {
  socket_map_.erase(socket_fd);
}
std::shared_ptr<Socket> Dispatcher::FindSocket(SOCKET socket_fd) {
  std::shared_ptr<Socket> socket(nullptr);
  SocketMap::iterator it = socket_map_.find(socket_fd);
  if (it != socket_map_.end())
    socket = it->second;
  return socket;
}
