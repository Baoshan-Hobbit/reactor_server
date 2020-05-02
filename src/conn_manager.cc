/*************************************************************************
  > File Name: conn_manager.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月30日 星期四 10时53分40秒
 ************************************************************************/

#include "src/conn_manager.h"
#include <memory>
#include "src/conn.h"
#include "src/types.h"

ConnManager& ConnManager::GetInstance() {
  static ConnManager conn_manager;
  return conn_manager;
}

void ConnManager::RegisterConn(SOCKET socket_fd, std::shared_ptr<Conn> conn) {
  conn_map_.insert(std::make_pair(socket_fd, conn));
}

void ConnManager::UnregisterConn(SOCKET socket_fd) {
  conn_map_.erase(socket_fd);
}

std::shared_ptr<Conn> ConnManager::FindConn(SOCKET socket_fd) {
  std::shared_ptr<Conn> conn(nullptr);
  ConnMap::iterator it = conn_map_.find(socket_fd);
  if (it != conn_map_.end())
    conn = it->second;
  return conn;
}
