/*************************************************************************
  > File Name: Socket.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月21日 星期二 17时20分30秒
 ************************************************************************/

#ifndef REACTOR_SERVER_SOCKET_H_
#define REACTOR_SERVER_SOCKET_H_

#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>

#include "src/utils.h"
#include "src/types.h"
#include "src/macro.h"

// 压根不是为纯client封装的类,socket的状态变化和检测是否可读可写
// 由epoll代劳,今儿调用OnRead(), OnWrite(), OnClose()函数
class Socket {
 public:
  Socket();
  // 析构不需要再调用Close(),因为Close()函数无非就做两件事:
  // 1. unregister
  // 2. close socket
  // 在进程结束时系统自动回收资源,不需要unregister;
  // 进程结束socket正常close,发送FIN,如ctrl+c时
  // 由于使用shared_ptr存储Socket对象,当从dispatcher中unregister后计数变为0,
  // 会调用析构函数,如果析构函数调用了Close(),会导致释放两次socket的奇怪bug
  //
  // 保守原则写代码,不确定的不要瞎用
  //~Socket() { Close(); }

  int Listen(const IP& server_ip, PORT server_port, 
      std::shared_ptr<CallbackWrapper> callback);
  int Connect(const IP& server_ip, PORT server_port, 
      std::shared_ptr<CallbackWrapper> callback); 
  void Close();

  int Send(const uint8_t* buf, int len);
  int Recv(uint8_t* buf, int len);

  void OnRead();
  void OnWrite();
  void OnClose();

  SOCKET get_socket_fd() const { return socket_fd_; }
  void set_callback(std::shared_ptr<CallbackWrapper> callback) { callback_ = callback; }
  void set_socket_fd(SOCKET fd) { socket_fd_ = fd; }
  void set_state(SOCKET_STATE state) { state_ = state; }
  void set_client_ip(IP client_ip) { client_ip_ = client_ip; }
  void set_client_port(PORT client_port) { client_port_ = client_port; }
  
  // 对对象有效,对指针不管用
  DISALLOW_COPY_AND_ASSIGN(Socket);

 private:
  void Accept();

  int SetReuseAddr();
  int SetNonBlock();
  int SetNonDelay();
  void SetAddr(sockaddr_in* pAddr);
  void SetSendBufSize(uint32_t send_size);
  void SetRecvBufSize(uint32_t recv_size);

 private:
  SOCKET socket_fd_;
  SOCKET_STATE state_;

  std::shared_ptr<CallbackWrapper> callback_;

  IP server_ip_;
  PORT server_port_;
  IP client_ip_;
  PORT client_port_;
};

#endif // REACTOR_SERVER_SOCKET_H_
