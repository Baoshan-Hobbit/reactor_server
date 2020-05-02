/*************************************************************************
  > File Name: conn_manager.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月30日 星期四 10时47分10秒
 ************************************************************************/

#ifndef REACTOR_SERVER_CONN_MANAGER_H_
#define REACTOR_SERVER_CONN_MANAGER_H_

#include <map>
#include <memory>
#include "src/conn.h"
#include "src/types.h"
#include "src/macro.h"

class ConnManager {
 public:
  static ConnManager& GetInstance(); 
  void RegisterConn(SOCKET socket_fd, std::shared_ptr<Conn> conn);
  void UnregisterConn(SOCKET socket_fd);
  std::shared_ptr<Conn> FindConn(SOCKET socket_fd);

  DISALLOW_COPY_AND_ASSIGN(ConnManager);

 private:
  ConnManager() {}

 private:
  typedef std::map<SOCKET, std::shared_ptr<Conn>> ConnMap;
  ConnMap conn_map_;
};

#endif // REACTOR_SERVER_CONN_MANAGER_H_
