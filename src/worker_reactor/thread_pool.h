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

class TextResponse : public Response {
 public:
  TextResponse(std::string content) : content_(content) {}
  void* get_content() const override{ return (void*)(content_.c_str()); }
  DISALLOW_COPY_AND_ASSIGN(TextResponse);

 private:
  std::string content_;
};

std::shared_ptr<Response> SellFruit(const std::string& name);

class Task {
 public:
  virtual ~Task() {}
  virtual std::shared_ptr<Response> Run() = 0;
};

class SellTask : public Task {
 public:
  //构造函数中需要SellType,隐含外部传进的参数被定义为此类型,因此定义为public
  typedef std::shared_ptr<Response> (*SellType)(const std::string&); 

  SellTask(SellType sell, const std::string& data) : sell_(sell), data_(data) {}
  std::shared_ptr<Response> Run() override { 
    std::shared_ptr<Response> result = sell_(data_); //TODO: 当改造成智能指针
    return result;
  }

 private:
  std::string data_;
  SellType sell_;
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
