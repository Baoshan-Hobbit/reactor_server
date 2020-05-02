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

//不宜将所有的辅助函数或辅助类都放入utils.h中,要避免两个头文件双向依赖,如若将GetSocket()放入utils.h,则socket.h和utils.h双向依赖
//
class CallbackWrapper {
 public:
  CallbackWrapper(callback_t callback_func, void* callback_data) : 
      callback_func_(callback_func), callback_data_(callback_data) {}

  void Run(SOCKET socket_fd, MSG_TYPE type) {
    callback_func_(callback_data_, socket_fd, type);
  }
  DISALLOW_COPY_AND_ASSIGN(CallbackWrapper);

 private:
  callback_t callback_func_;
  void* callback_data_;
};

// 定义为inline,否则会报错多次定义
inline int GetErrorCode() { return errno; }
// 注意形参不能与errno重名
inline bool IsBlock(int error_code) {
  return ((error_code == EINPROGRESS) || (error_code == EWOULDBLOCK));
} 

class Buffer {
 public:
  Buffer(int capacity) : capacity_(capacity), write_offset_(0), buffer_(nullptr) { buffer_ = new char[capacity_]; }
  ~Buffer() {
    if (buffer_) {
      free(buffer_);
      buffer_ = nullptr;
    }
  }
  DISALLOW_COPY_AND_ASSIGN(Buffer);

  int ReadOut(char* buf, int len);
  int Read(char* buf, int len);
  int Write(const char* data, int len);
  char* get_buffer() { return buffer_; }
  //char* GetOffset() { return buffer_ + write_offset_; }
  int get_offset() { return write_offset_; }
  //int get_capacity() { return capacity_; }

 private:
  void Expand(int size);

 private:
  int capacity_;
  char* buffer_;
  int write_offset_;
// TODO: 扩容,环形缓冲区
// 扩容的方式: new,copy,delete; vector; streambuf? 
};

const int MAX_SOCKET_BUF_SIZE = 8;
#endif // REACTOR_SERVER_UTILS_H_
