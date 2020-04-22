/*************************************************************************
  > File Name: Socket.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月21日 星期二 17时20分30秒
 ************************************************************************/

#ifndef REACTOR_SERVER_SOCKET_H_
#define REACTOR_SERVER_SOCKET_H_

#include <errno.h>
#include "src/utils.h"
#include "src/types.h"
#include "src/macro.h"

class Socket {
 public:
  Socket();

  int Listen(const IP& server_ip, PORT server_port, 
      CallbackWrapper* callback);
  int Connect(const IP& server_ip, PORT server_port, 
      CallbackWrapper* callback); 
  void Close();

  int Send(void* buf, int len);
  int Recv(void* buf, int len);

  void OnRead();
  void OnWrite();
  void OnClose();

  void set_callback(CallbackWrapper* callback) { callback_ = callback; }
  void set_socket_fd(SOCKET fd) { socket_fd_ = fd; }
  void set_state(SOCKET_STATE state) { state_ = state; }
  void set_client_ip(IP client_ip) { client_ip_ = client_ip; }
  void set_client_port(PORT client_port) { client_port_ = client_port; }

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

  CallbackWrapper* callback_;

  IP server_ip_;
  PORT server_port_;
  IP client_ip_;
  PORT client_port_;
};

#endif // REACTOR_SERVER_SOCKET_H_
