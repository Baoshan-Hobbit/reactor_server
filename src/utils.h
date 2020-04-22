/*************************************************************************
  > File Name: utils.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月21日 星期二 18时10分12秒
 ************************************************************************/

#ifndef REACTOR_SERVER_UTILS_H_
#define REACTOR_SERVER_UTILS_H_

#include "src/types.h"
#include "src/macro.h"

class CallbackWrapper {
 public:
  CallbackWrapper(callback_t callback_func, void* callback_data) : 
      callback_func_(callback_func), callback_data_(callback_data) {}

  void Run(SOCKET socket_fd, uint8_t type) {
    callback_func_(callback_data_, socket_fd, type);
  }
  DISALLOW_COPY_AND_ASSIGN(CallbackWrapper);

 private:
  callback_t callback_func_;
  void* callback_data_;
};

int GetErrorCode() { return errno; }

#endif // REACTOR_SERVER_UTILS_H_
