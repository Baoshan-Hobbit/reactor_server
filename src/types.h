/*************************************************************************
  > File Name: types.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月21日 星期二 17时23分47秒
 ************************************************************************/

#ifndef REACTOR_SERVER_TYPES_H_
#define REACTOR_SERVER_TYPES_H_

#include <stdint.h>
#include <string>

typedef int SOCKET;
typedef uint16_t PORT;
typedef std::string IP;

enum SOCKET_STATE {
  IDLE,
  LISTENING, 
  CONNECTING,
  CONNECTED,
  CLOSING
};

enum MSG_TYPE {
  NETLIB_MSG_CONNECT, 
  NETLIB_MSG_CONFIRM,
  NETLIB_MSG_CLOSE,
  NETLIB_MSG_READ,
  NETLIB_MSG_WRITE
};

enum MSG_SOURCE {
  FROM_CLIENT,
  FROM_SERVER
};

const int NETLIB_ERROR = -1; // Socket类错误码,非常简化
const int INVALID_SOCKET = -1; // 创建socket失败,未能产生socket_fd
const int SOCKET_ERROR = -1; // socket函数调用失败
const int EPOLL_ERROR = -1; // epoll函数调用失败

typedef void (*callback_t)(void* callback_data, SOCKET socket_fd, MSG_TYPE type);

#endif // REACTOR_SERVER_TYPES_H_
