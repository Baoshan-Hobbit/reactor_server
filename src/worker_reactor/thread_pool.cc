/*************************************************************************
  > File Name: thread_pool.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月16日 星期四 09时29分53秒
 ************************************************************************/

#include "src/worker_reactor/thread_pool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <deque>
#include <memory>

void* ThreadPool::Run(void* arg) {
  ThreadPool* ptr = (ThreadPool*)arg;
  while (true) {
    pthread_t tid = pthread_self();
    pthread_mutex_lock(&mutex_);
    while(task_queue_.empty())
      pthread_cond_wait(&task_queue_cond_, &mutex_);
    std::shared_ptr<Task> task = task_queue_.front();
    task_queue_.pop_front();
    // 有时反应不过来会出现重复写的情形,原因待查
    // static成员函数可以充当pthread_create的函数参数但只能调用其静态成员,
    // 这里传入this是为了调用非静态成员
    printf("pool: %s, %u execute task.\n", ptr->name_.c_str(), 
        (unsigned int)tid);
    pthread_mutex_unlock(&mutex_);
    std::shared_ptr<Response> response = task->Run();
    //sleep(rand() % 5); // 便于观察,debug用
    ptr->AddResponse(response);
  }
}

void ThreadPool::AddResponse(std::shared_ptr<Response> response) {
  pthread_mutex_lock(&response_mutex_);
  response_queue_.push_back(response);
  pthread_mutex_unlock(&response_mutex_);
}

std::shared_ptr<Response> ThreadPool::GetResponse() {
  std::shared_ptr<Response> response(nullptr);
  pthread_mutex_lock(&response_mutex_);
  if (!response_queue_.empty()) {
    response = response_queue_.front();
    response_queue_.pop_front();
  }
  pthread_mutex_unlock(&response_mutex_);
  return response;
}

std::deque<std::shared_ptr<Task> > ThreadPool::task_queue_;
std::deque<std::shared_ptr<Response> > ThreadPool::response_queue_;
pthread_mutex_t ThreadPool::mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ThreadPool::response_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::task_queue_cond_ = PTHREAD_COND_INITIALIZER;

void FixedThreadPool::AddTask(std::shared_ptr<Task> task) {
  pthread_mutex_lock(&mutex_);
  task_queue_.push_back(task);
  pthread_mutex_unlock(&mutex_);

  pthread_cond_signal(&task_queue_cond_);
}

void FixedThreadPool::Create() {
  for (int i=0; i<core_pool_size_; ++i) {
    pthread_t thread;
    pthread_create(&thread, nullptr, Run, (void*)this);
    pthread_detach(thread);
    thread_pool_.push_back(thread);
  } 
}

