/*************************************************************************
  > File Name: socket.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年05月02日 星期六 16时59分58秒
 ************************************************************************/

#include "src/worker_reactor/socket.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h> // fcntl()
#include <fcntl.h> // fcntl
#include <arpa/inet.h> // ntohl()
#include <netinet/in.h> // sockaddr_in
#include <netinet/tcp.h> // TCP_NODELAY
#include <stdio.h>
#include <string.h> // memset()
#include <memory>

#include "src/worker_reactor/dispatcher.h"
#include "src/types.h"
#include "src/utils.h"

Socket::Socket() : socket_fd_(INVALID_SOCKET), state_(IDLE), 
  callback_(nullptr) {}

int Socket::Listen(const IP& server_ip, PORT server_port, 
    std::shared_ptr<CallbackWrapper> callback) {
  server_ip_ = server_ip;
  server_port_ = server_port;
  callback_ = callback;

  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ == INVALID_SOCKET) {
    printf("create socket failed. error code: %d\n", GetErrorCode());
    return NETLIB_ERROR; 
  }

  int ret = SetReuseAddr();
  if (ret == SOCKET_ERROR) {
    printf("reuse addr failed, error code: %d\n", GetErrorCode());
    Close();
    return NETLIB_ERROR;
  }

  ret = SetNonBlock();
  if (ret == SOCKET_ERROR) {
    printf("set non-block failed, error code: %d\n", GetErrorCode());
    Close();
    return NETLIB_ERROR;
  }

  sockaddr_in server_addr;
  SetAddr(&server_addr);
  ret = bind(socket_fd_, (sockaddr*)&server_addr, sizeof(server_addr));
  if (ret == SOCKET_ERROR) {
    printf("bind failed, error code: %d\n", GetErrorCode());
    Close();
    return NETLIB_ERROR;
  }

  int backlog = 64;
  ret = listen(socket_fd_, backlog);
  // 禁用异常时,事务性调用外部接口不可避免使用错误码,
  // 且需要手动传递错误到上层
  // 增加很多与业务逻辑无关的判断
  // 很烦但没有办法
  if (ret == SOCKET_ERROR) {
    printf("listen failed, error code: %d\n", GetErrorCode());
    Close();
    return NETLIB_ERROR; // 直接返回错误,中断正常执行路径
  }

  state_ = LISTENING;
  printf("listening on %s:%d\n", server_ip_.c_str(), server_port_);

  //TODO: Register
  Dispatcher& dispatcher = Dispatcher::GetInstance();
  dispatcher.AddEvent(socket_fd_);
  std::shared_ptr<Socket> this_socket(this);
  dispatcher.RegisterSocket(socket_fd_, this_socket);
  return ret;
}

int Socket::Connect(const IP& server_ip, PORT server_port, 
    std::shared_ptr<CallbackWrapper> callback) {
  server_ip_ = server_ip;
  server_port_ = server_port;
  callback_ = callback;

  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ == INVALID_SOCKET) {
    printf("create socket failed. error code: %d\n", GetErrorCode());
    return NETLIB_ERROR; 
  }

  int ret = SetNonDelay();
  if (ret == SOCKET_ERROR) {
    printf("set non-delay failed, error code: %d\n", GetErrorCode());
    Close();
    return NETLIB_ERROR;
  }

  ret = SetNonBlock();
  if (ret == SOCKET_ERROR) {
    printf("set non-block failed, error code: %d\n", GetErrorCode());
    Close();
    return NETLIB_ERROR;
  }

  sockaddr_in server_addr;
  SetAddr(&server_addr);
  ret = connect(socket_fd_, (sockaddr*)&server_addr, sizeof(server_addr));
  if (ret == SOCKET_ERROR) {
    int err_code = GetErrorCode();
    if (IsBlock(err_code)) {
      ret = 0;
    } else {
      printf("connect failed, error code: %d\n", err_code);
      Close();
      return NETLIB_ERROR;
    }
  }
  state_ = CONNECTING;
  printf("connecting to %s:%d\n", server_ip_.c_str(), server_port_);

  Dispatcher& dispatcher = Dispatcher::GetInstance();
  dispatcher.AddEvent(socket_fd_);
  std::shared_ptr<Socket> this_socket(this);
  dispatcher.RegisterSocket(socket_fd_, this_socket);

  return ret;
}

// 为什么被调用两次?
void Socket::Close() {
  state_ = CLOSING;
  Dispatcher& dispatcher = Dispatcher::GetInstance();
  if (dispatcher.FindSocket(socket_fd_)) {
    //printf("socket found, to close\n");
    dispatcher.RemoveEvent(socket_fd_);
    dispatcher.UnregisterSocket(socket_fd_); 
  } 
  //else {
  //  printf("socket not found, to close\n");
  //}
  close(socket_fd_);
  printf("socket: %d closed.\n", socket_fd_);
}

int Socket::Send(const uint8_t* buf, int len) {
  if (state_ != CONNECTED)
    return NETLIB_ERROR;
  int ret = write(socket_fd_, buf, len);
  //if (ret == SOCKET_ERROR) {
    //int err_code = GetErrorCode();
    //if (IsBlock(err_code)) {
    //  ret = 0;
    //} else {
    //  printf("send failed, error code: %d\n", err_code);
    //}
  //}
  if (ret == SOCKET_ERROR && !IsBlock(GetErrorCode()))
    printf("send failed, error code: %d\n", GetErrorCode());
  return ret; 
}

int Socket::Recv(uint8_t* buf, int len) {
  if (state_ != CONNECTED)
    return NETLIB_ERROR;
  int ret = read(socket_fd_, buf, len);
  if (ret == SOCKET_ERROR && !IsBlock(GetErrorCode()))
    printf("recv failed, error code: %d\n", GetErrorCode());
  return ret;
}

void Socket::OnRead() {
  if (state_ == LISTENING) {
    Accept();
  } else {
    u_long avail = 0;
    //if ((ioctl(socket_fd_, FIONREAD, &avail) == SOCKET_ERROR) || (avail == 0)) {
    ioctl(socket_fd_, FIONREAD, &avail);
    if (avail == 0) {
      printf("onread detect close\n");
      callback_->Run(socket_fd_, NETLIB_MSG_CLOSE);   
    } else {
      callback_->Run(socket_fd_, NETLIB_MSG_READ); 
    }
  }
}

void Socket::OnWrite() {
  if (state_ == CONNECTING) {
    int err = 0;
    socklen_t len = sizeof(err);
    getsockopt(socket_fd_, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err) {
      printf("second connecting failed.\n");
      callback_->Run(socket_fd_, NETLIB_MSG_CLOSE);
    } else {
      state_ = CONNECTED;
      printf("connected to %s:%d\n", server_ip_.c_str(), server_port_);
      callback_->Run(socket_fd_, NETLIB_MSG_CONFIRM);
    }
  } else {
    callback_->Run(socket_fd_, NETLIB_MSG_WRITE);
  }
}

void Socket::OnClose() {
  printf("socket onclose\n");
  callback_->Run(socket_fd_, NETLIB_MSG_CLOSE);
}

void Socket::Accept() {
  SOCKET conn_fd = 0;
  sockaddr_in client_addr;
  socklen_t addr_len = sizeof(sockaddr_in);
  char client_ip[16];
  while ((conn_fd = accept(socket_fd_, (sockaddr*)&client_addr, &addr_len)) != 
      INVALID_SOCKET) {
    uint32_t ip = ntohl(client_addr.sin_addr.s_addr);
    uint16_t client_port = ntohs(client_addr.sin_port);
    snprintf(client_ip, sizeof(client_ip), "%d.%d.%d.%d", ip>>24, (ip>>16)&0xFF, 
       (ip>>8)&0xFF, ip&0xFF);
    printf("accept conn, socket fd=%d from %s:%d\n", conn_fd, client_ip, 
        client_port);

    // bug, no delete
    std::shared_ptr<Socket> conn_socket = std::make_shared<Socket>();

    conn_socket->set_socket_fd(conn_fd);
    conn_socket->set_state(CONNECTED);
    //conn_socket->set_callback(callback_);

    conn_socket->set_client_ip(client_ip);
    conn_socket->set_client_port(client_port);

    conn_socket->SetNonDelay();
    conn_socket->SetNonBlock();

    Dispatcher& dispatcher = Dispatcher::GetInstance();
    dispatcher.AddEvent(conn_fd);
    dispatcher.RegisterSocket(conn_fd, conn_socket);

    callback_->Run(conn_fd, NETLIB_MSG_CONNECT); 
 }
}

int Socket::SetReuseAddr() {
  int reuse = 1;
  int ret = setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, 
      sizeof(reuse));
  return ret;
}

int Socket::SetNonBlock() {
	int ret = fcntl(socket_fd_, F_SETFL, O_NONBLOCK | fcntl(socket_fd_, F_GETFL));
  return ret;
}

int Socket::SetNonDelay() {
  int nodelay = 1;
	int ret = setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &nodelay, 
      sizeof(nodelay));
  return ret;
}

void Socket::SetAddr(sockaddr_in* addr) {
  // 只声明未定义,addr指向的内存数据是不确定的,需要清空后赋值
	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(server_port_);
	addr->sin_addr.s_addr = inet_addr(server_ip_.c_str());
	if (addr->sin_addr.s_addr == INADDR_NONE)
    printf("gethostbyname failed, ip=%s\n", server_ip_.c_str());
}

void Socket::SetSendBufSize(uint32_t send_size)
{
	int ret = setsockopt(socket_fd_, SOL_SOCKET, SO_SNDBUF, &send_size, 4);
	if (ret == SOCKET_ERROR)
		printf("set send_buf_size failed for fd=%d\n", socket_fd_);

	socklen_t len = 4;
	int size = 0;
	getsockopt(socket_fd_, SOL_SOCKET, SO_SNDBUF, &size, &len);
	printf("socket=%d send_buf_size=%d\n", socket_fd_, size);
}

void Socket::SetRecvBufSize(uint32_t recv_size)
{
	int ret = setsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, &recv_size, 4);
	if (ret == SOCKET_ERROR) {
		printf("set recv_buf_size failed for fd=%d\n", socket_fd_);
	}

	socklen_t len = 4;
	int size = 0;
	getsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, &size, &len);
	printf("socket=%d recv_buf_size=%d\n", socket_fd_, size);
}
