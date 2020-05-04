/*************************************************************************
  > File Name: dispatcher.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月22日 星期三 15时06分26秒
 ************************************************************************/

#ifndef REACTOR_SERVER_DISPATCHER_H_
#define REACTOR_SERVER_DISPATCHER_H_

#include <unistd.h> // 文件句柄相关
#include <map>
#include <memory>
#include "src/types.h"
#include "src/worker_reactor/socket.h"
#include "src/worker_reactor/thread_pool.h"
#include "src/macro.h"

// TODO: 双向依赖待查
class Dispatcher {
 public:
  // 资源用完关闭
  ~Dispatcher() { close(epfd_); }
  static Dispatcher& GetInstance();
  void Start();

  void AddEvent(SOCKET socket_fd);
  void RemoveEvent(SOCKET socket_fd);
  void RegisterSocket(SOCKET socket_fd, std::shared_ptr<Socket> socket);
  void UnregisterSocket(SOCKET socket_fd);
  std::shared_ptr<Socket> FindSocket(SOCKET socket_fd);
  std::shared_ptr<ThreadPool> get_threadpool() { return thread_pool_; }

  DISALLOW_COPY_AND_ASSIGN(Dispatcher);

 private:
  Dispatcher(int event_num, uint32_t wait_timeout, int threadpool_size);
  void CheckLoop();

 private:
  typedef std::map<SOCKET, std::shared_ptr<Socket> > SocketMap;

  int event_num_;
  int epfd_;
  uint32_t wait_timeout_;
  int threadpool_size_;
  bool is_running_;
  SocketMap socket_map_;
  std::shared_ptr<ThreadPool> thread_pool_;
};

#endif // REACTOR_SERVER_DISPATCHER_H_
