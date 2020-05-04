/*************************************************************************
  > File Name: conn.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月29日 星期三 12时32分38秒
 ************************************************************************/

#include "src/simple_reactor/conn.h"
#include <stdio.h>
#include <string.h>
#include <memory>
#include "src/simple_reactor/socket.h"
#include "src/simple_reactor/dispatcher.h"
#include "src/utils.h"

std::shared_ptr<Socket> GetSocket(SOCKET socket_fd) {
  Dispatcher& dispatcher = Dispatcher::GetInstance();
  std::shared_ptr<Socket> socket = dispatcher.FindSocket(socket_fd);
  return socket;
}

// 通过socket发送消息
// 目标: 尽可能的发送,要么将out_buffer_中的数据发送完,要么发送不完,等待下次再发送
// 本质上是读取out_buffer,允许读取不完
int Conn::Send(const uint8_t* data, int len) {
  if (is_busy_) {
    out_buffer_->Write(data, len);
    printf("busy, write to buffer.\n");
    return len;
  }
  int offset = 0;
  int remain = len;
  std::shared_ptr<Socket> socket = GetSocket(socket_fd_);
  if (!socket)
    return -1;
  while (remain > 0) {
    int send_size = remain;
    if (send_size > MAX_SOCKET_BUF_SIZE)
      send_size = MAX_SOCKET_BUF_SIZE;
    int ret = socket->Send(data + offset, send_size);
    if (ret < 0)
      break;
    offset += send_size;
    remain -= send_size;
  }
  if (remain > 0) {
    is_busy_ = true;
    out_buffer_->Write(data + offset, remain);
    printf("remain %d bytes, write to buffer.\n", remain);
  } else {
    printf("all sent\n");
  }
  return len;
}

void Conn::OnWrite() {
  if (!is_busy_)
    return;
  std::shared_ptr<Socket> socket = GetSocket(socket_fd_);
  if (!socket)
    return;
  uint8_t temp[MAX_SOCKET_BUF_SIZE];
  while (out_buffer_->get_offset() > 0) {
    memset(temp, 0, MAX_SOCKET_BUF_SIZE);
    printf("onwrite executed\n");
    // 先拷贝读取,并不取出Buffer内容,以防Send失败无法恢复buffer
    int send_size = out_buffer_->Read(temp, MAX_SOCKET_BUF_SIZE);
    int ret = socket->Send(temp, send_size); // ret == send_size if normal
    if (ret < 0)
      break;
    out_buffer_->ReadOut(nullptr, ret);
  }
  if (out_buffer_->get_offset() == 0)
    is_busy_ = false;
}
