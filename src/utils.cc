/*************************************************************************
  > File Name: utils.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月26日 星期日 18时58分34秒
 ************************************************************************/

#include "src/utils.h"
#include "src/types.h"
#include <string.h>

// 将buffer_中的数据读取到buf中,期望读取len字节
// len == 用于接收数据的buf的大小
// 读取的目标: 尽可能的读,要么将buffer_中的数据读完(len >= write_offet_),要么读不完但是将buf填满(len < write_offset_),剩下没读的
// 下次再读
int Buffer::ReadOut(char* buf, int len) {
  int read_len = Read(buf, len);
  memmove(buffer_, buffer_+read_len, write_offset_-read_len);
  write_offset_ -= read_len;
  return read_len;
}

int Buffer::Read(char* buf, int len) {
  if (len > write_offset_)
    len = write_offset_;
  //strncpy(buf, buffer_, len);
  if (buf)
    memcpy(buf, buffer_, len);
  return len;
}

// 将data中的数据写入到buffer_中,期望写入len字节
// len == 要写入的data的字节数
// 写入的目标: 必须写完,即必须将data完全写入buffer_中
// 若len <= avail,正常写入,若len > avail,扩容后写入
int Buffer::Write(const char* data, int len) {
  if (data) {
    int avail = capacity_ - write_offset_;
    if (len > avail)
      Expand(len - avail);
    //strncpy(buffer_+write_offset_, data, len);
    memcpy(buffer_+write_offset_, data, len);
  }
  write_offset_ += len;
  return len;
}

void Buffer::Expand(int size) {
  capacity_ += (size + size / 4);
  char* new_buffer = new char[capacity_];
  memcpy(new_buffer, buffer_, write_offset_);
  delete[] buffer_;
  buffer_ = new_buffer;
}

