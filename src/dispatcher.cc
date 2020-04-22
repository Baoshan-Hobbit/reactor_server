/*************************************************************************
  > File Name: dispatcher.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月22日 星期三 15时12分42秒
 ************************************************************************/

#include "dispatcher.h"

#include <sys/epoll.h>
#include <stdio.h>

#include "src/socket.h"
#include "src/types.h"
#include "src/utils.h"

Dispatcher& Dispatcher::GetInstance() {
  int event_num = 1024;
  uint32 wait_timeout = 10;
  static Dispatcher dispatcher(event_num, wait_timeout);
  return dispatcher;
}

Dispatcher::Dispatcher(int event_num, uint32_t wait_timeout) : 
                      event_num_(event_num), 
                      wait_timeout_(wait_timeout), 
                      is_running(false) { 
  epfd_ = epoll_create(event_num_); 
  if (epfd == EPOLL_ERROR)
    printf("epoll init failed\n");
}

void Dispatcher::Start() {
  epoll_event events[event_num_];
  if (is_running_) 
    return;
  is_running_ = true;
  while (is_running_) {
    ready_fd_num = epoll_wait(epfd_, events, event_num_, wait_timeout_);
    for (int i=0; i<ready_fd_num; ++i) {
      SOCKET socket_fd = events[i].data.fd;
      Socket* socket = FindSocket(socket_fd);
      if (!socket)
        continue;
      if (events[i].events & EPOLLIN)
        socket->OnRead();
      if (events[i].events & EPOLLOUT)
        socket->OnWrite();
      if (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP))
        socket->OnClose();
    }
  }
}

void Dispatcher::AddEvent(SOCKET socket_fd) {
  epoll_event ev;
  ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLPRI | EPOLLERR | EPOLLHUP;
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

void Dispatcher::RegisterSocket(SOCKET socket_fd, Socket* socket) {
  socket_map_.insert(std::make_pair(socket_fd, socket)); 
}
void Dispatcher::UnregisterSocket(SOCKET socket_fd) {
  socket_map_.erase(socket_fd);
}
Socket* Dispatcher::FindSocket(SOCKET socket_fd) {
  Socket* socket = nullptr;
  SocketMap::iterator it = socket_map_.find(socket_fd);
  if (it != socket_map_.end())
    socket = it->second;
  return socket;
}
