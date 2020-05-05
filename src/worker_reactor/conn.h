/*************************************************************************
  > File Name: conn.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月29日 星期三 12时27分31秒
 ************************************************************************/

#ifndef REACTOR_SERVER_CONN_H_
#define REACTOR_SERVER_CONN_H_

#include <memory>
#include "src/types.h"
#include "src/utils.h"
#include "src/worker_reactor/socket.h"

std::shared_ptr<Socket> GetSocket(SOCKET socket_fd);

class Conn {
 public:
  virtual ~Conn() {} 
  int Send(const uint8_t* data, int len);
  void OnWrite();
  void WriteOutBuffer(const uint8_t* data, int len) {
    out_buffer_->Write(data, len);
  }
  virtual void OnRead() = 0;
  virtual void OnClose() = 0;
  virtual int GetResponseSize(void* response) = 0;

 protected:
  Conn(int buf_capacity, SOCKET socket_fd) : socket_fd_(socket_fd),
                                in_buffer_(new Buffer(buf_capacity)), 
                                out_buffer_(new Buffer(buf_capacity)), 
                                is_busy_(false) {}

 protected:
  SOCKET socket_fd_;
  std::unique_ptr<Buffer> in_buffer_;
  std::unique_ptr<Buffer> out_buffer_;
  bool is_busy_;
};

#endif // REACTOR_SERVER_CONN_H_
