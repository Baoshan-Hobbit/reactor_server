/*************************************************************************
  > File Name: thread_pool.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月17日 星期五 15时53分43秒
 ************************************************************************/

#ifndef CPLUS_PRACTICE_CONCURRENT_THREADPOOL_H_
#define CPLUS_PRACTICE_CONCURRENT_THREADPOOL_H_

#include <pthread.h>
#include <limits.h> // INT_MAX
#include <string>
#include <deque>
#include <memory>
#include <vector>
#include "src/types.h"
#include "src/macro.h"

// Response基类,不同的业务类型可能返回不同的content,如字符串,图片,文件等
class Response {
 public:
  virtual ~Response() {}
  virtual void* get_content() const = 0; 
  SOCKET get_socketfd() { return socket_fd_; }

 protected:
  Response() {} // 权宜之计,其实不行也要无参构造
  Response(SOCKET socket_fd) : socket_fd_(socket_fd) {}

 private:
  SOCKET socket_fd_;
};

class Task {
 public:
  virtual ~Task() {}
  virtual std::shared_ptr<Response> Run() = 0;
};

class ThreadPool {
 public:
  ThreadPool(int core_pool_size, int max_pool_size, int queue_size) : 
             core_pool_size_(core_pool_size), 
             max_pool_size_(max_pool_size), 
             queue_size_(queue_size) {} 
  virtual ~ThreadPool() {}

  virtual void AddTask(std::shared_ptr<Task> task) = 0;
  void set_name(const std::string& name) { name_ = name; }
  std::shared_ptr<Response> GetResponse();

 protected:
  virtual void Create() = 0;
  static void* Run(void* arg);
  void AddResponse(std::shared_ptr<Response> response);

 protected:
  int core_pool_size_;
  int max_pool_size_;
  int queue_size_;

  static std::deque<std::shared_ptr<Task> > task_queue_;
  static std::deque<std::shared_ptr<Response> > response_queue_;
  static pthread_mutex_t mutex_;
  static pthread_mutex_t response_mutex_;
  static pthread_cond_t task_queue_cond_;
  std::vector<pthread_t> thread_pool_;
  std::string name_;
};

class FixedThreadPool : public ThreadPool {
 public:
  explicit FixedThreadPool(int pool_size) : ThreadPool(pool_size, pool_size, 
                                              INT_MAX) { Create(); } 
  void AddTask(std::shared_ptr<Task> task) override;
  DISALLOW_COPY_AND_ASSIGN(FixedThreadPool);

 private:
  void Create() override;
};


#endif //CPLUS_PRACTICE_CONCURRENT_THREADPOOL_H_
