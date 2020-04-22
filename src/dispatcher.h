/*************************************************************************
  > File Name: dispatcher.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月22日 星期三 15时06分26秒
 ************************************************************************/

#ifndef REACTOR_SERVER_DISPATCHER_H_;
#define REACTOR_SERVER_DISPATCHER_H_;

#include <hash_map>
#include "src/types.h"
#include "src/socket.h"

class Dispatcher {
 public:
  static Dispatcher& GetInstance();
  void Start();
  void Stop() { is_running_ = false; }

  void AddEvent(SOCKET socket_fd);
  void RemoveEvent(SOCKET socket_fd);
  void RegisterSocket(SOCKET socket_fd, Socket* socket);
  void UnregisterSocket(SOCKET socket_fd);

 private:
  Dispatcher(int event_num);
  Socket* FindSocket(SOCKET socket_fd);

 private:
  typedef hash_map<SOCKET Socket*> SocketMap;

  int event_num_;
  int epfd_;
  uint32_t wait_timeout_;
  bool is_running_;
  SocketMap socket_map_;
};

#endif // REACTOR_SERVER_DISPATCHER_H_
