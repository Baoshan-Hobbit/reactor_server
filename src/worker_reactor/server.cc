/*************************************************************************
  > File Name: server.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年05月02日 星期六 16时55分12秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <memory>

#include "src/worker_reactor/socket.h"
#include "src/worker_reactor/dispatcher.h"
#include "src/types.h"
#include "src/utils.h"
#include "src/macro.h"
#include "src/worker_reactor/conn.h"
#include "src/worker_reactor/conn_manager.h"
#include "src/worker_reactor/thread_pool.h"


/* 总结: 业务逻辑需要实现的部分: 
 * 1. requestTaskData, Response的包结构
 * 2. 处理具体请求的函数
 * 3. 对具体Task的实现,包含处理请求的函数成员和requestTaskData
 * 4. 拆包
 * 5. 组装包
 */

// 对RequstTaskData类的业务实现,包含socket_fd和request字符串
class RequestTaskData {
 public:
  RequestTaskData(SOCKET socket_fd, const std::string& request): 
    socket_fd_(socket_fd), request_(request) {}
  SOCKET get_socketfd() { return socket_fd_; }
  std::string get_request() const { return request_; }

 private:
  SOCKET socket_fd_;
  std::string request_;
};

// 对Response类的业务实现
class SimpleResponse : public Response {
 public:
  SimpleResponse(SOCKET socket_fd, std::string content) : Response(socket_fd), 
                                                          content_(content) {}
  // 返回一个对象的内部成员的常量指针
  // void*返回值是考虑到多态,不同的Response实现其封装的content也
  // 不同
  void* get_content() const override { return (void*)(content_.c_str()); }

  DISALLOW_COPY_AND_ASSIGN(SimpleResponse);

 private:
  std::string content_;
};

// 对Task中的处理函数的业务实现
std::shared_ptr<Response> SellFruit(std::shared_ptr<RequestTaskData> data) {
  //printf("sell %s\n", name.c_str());
  std::shared_ptr<SimpleResponse> response = std::make_shared<SimpleResponse>(
      data->get_socketfd(), data->get_request());
  return response;
}

// 对RequestTask的业务实现,包含task的处理函数,待处理的请求数据
class RequestTask : public Task {
 public:
  //构造函数中需要SellType,隐含外部传进的参数被定义为此类型,因此定义为public
  typedef std::shared_ptr<Response> (*RequestHandle)(
      std::shared_ptr<RequestTaskData>); 

  RequestTask(RequestHandle request_handle, 
              std::shared_ptr<RequestTaskData> data) : 
    request_handle_(request_handle), data_(data) {}
  std::shared_ptr<Response> Run() override { 
    std::shared_ptr<Response> result = request_handle_(data_); //TODO: 当改造成智能指针
    return result;
  }

 private:
  RequestHandle request_handle_;
  std::shared_ptr<RequestTaskData> data_;
};

class ServerConn : public Conn {
 public:
  ServerConn(int buf_capacity, SOCKET socket_fd): 
     Conn(buf_capacity, socket_fd) {}
  void OnRead() override;
  void OnClose() override;
  int GetResponseSize(void* response) override;
  DISALLOW_COPY_AND_ASSIGN(ServerConn);
};

// 通过socket接收消息
// 目标: 接收所有的消息,然后写入in_buffer
void ServerConn::OnRead() {
  std::shared_ptr<Socket> socket = GetSocket(socket_fd_);
  if (!socket)
    return;
  //printf("youyouyou onread\n");
  uint8_t temp[MAX_SOCKET_BUF_SIZE];
  while (true) {
    memset(temp, 0, MAX_SOCKET_BUF_SIZE);
    // ret不一定等于期望读取的字节数
    // socket recv函数保证如果返回-1,则不写入接收的buffer中
    int ret = socket->Recv(temp, MAX_SOCKET_BUF_SIZE);
    //printf("ret: %d\n", ret);
    if (ret < 0)
      break;
    in_buffer_->Write(temp, ret); // TODO: temp用nullptr判断不合适
  }
 
  // 拆包的业务逻辑
  int content_len = in_buffer_->get_offset();
  if (content_len == 0)
    return;
  // 如果是char数组,用于接收的数组长度必须+1
  uint8_t request_buf[content_len + 1];
  // 清空内存,准备接收
  memset(request_buf, 0, sizeof(request_buf));
  int ret = in_buffer_->ReadOut(request_buf, content_len);
  //printf("after readout, offset: %d\n", in_buffer_->get_offset());
  //printf("after readout, buffer: %s\n", in_buffer_->get_buffer());
  std::string request_content((const char*)request_buf);
  //printf("request: %s, size: %d\n", request_content.c_str(), (int)request_content.size());

  // 将读取到的请求数据封装成task的业务逻辑
  std::shared_ptr<RequestTaskData> task_data = std::make_shared<RequestTaskData>(socket_fd_, request_content);
  std::shared_ptr<RequestTask> request_task(new RequestTask(SellFruit, task_data));
  
  // 将task递交给thread_pool
  Dispatcher& dispatcher = Dispatcher::GetInstance();
  dispatcher.PutTaskToThreadpool(request_task);
  printf("server handle conn socket: %d\n", socket_fd_);
}

void ServerConn::OnClose() {
  ConnManager& conn_manager = ConnManager::GetInstance();
  if (conn_manager.FindConn(socket_fd_))
    conn_manager.UnregisterConn(socket_fd_);

  std::shared_ptr<Socket> socket = GetSocket(socket_fd_);
  if (socket) {
    //printf("socket found, serverconn close socket\n");
    socket->Close();
  } 
  //else {
    //printf("socket not found in serverconn\n");
  //}
}

int ServerConn::GetResponseSize(void* response) {
  return strlen((const char*)response);
} 

void ConnCallback(void* callback_data, SOCKET socket_fd, MSG_TYPE type) {
  ConnManager& conn_manager = ConnManager::GetInstance();
  std::shared_ptr<Conn> server_conn = conn_manager.FindConn(socket_fd);
  if (!server_conn)
    return;

  switch (type) {
    //case NETLIB_MSG_CONNECT:
      // 握手成功后的逻辑
      //printf("server allowed connection.\n");
      //break;
    case NETLIB_MSG_READ:
      // 接收到请求后的逻辑,处理,得到响应
      {
        // 可验证LT模式下设置非阻塞即使没有数据也会频繁触发,占用cpu资源
        //printf("server prepare to read.\n");
        server_conn->OnRead();
      }
      break;
    case NETLIB_MSG_WRITE:
      // 注意多行代码switch要加{}表示作用域
      {      
        //printf("server prepare to write.\n");
        server_conn->OnWrite();
      }
      break;
    case NETLIB_MSG_CLOSE:
      // 关闭socket前的逻辑
      {
        printf("server prepare to close.\n");
        server_conn->OnClose();
      }
      break;
    default:
      break;
  }
}

void ListenCallback(void* callback_data, SOCKET socket_fd, MSG_TYPE type) {
  // 注意Buffer类的设计,不能将buffer_直接暴露出来给recv使用,因为其容量初始值
  // 有限,如果绕过Write()无法扩容,recv不断向其中写数据可能会溢出(segment fault)
  if (type == NETLIB_MSG_CONNECT) {
    std::shared_ptr<Socket> conn_socket = GetSocket(socket_fd);
    //CallbackWrapper conn_callback(ConnCallback, nullptr);
    //conn_socket->set_callback(&conn_callback);
    std::shared_ptr<CallbackWrapper> conn_callback = std::make_shared<CallbackWrapper>(ConnCallback, nullptr);
    conn_socket->set_callback(conn_callback);
    // bug here, new not delete
    std::shared_ptr<ServerConn> server_conn = std::make_shared<ServerConn>(10, socket_fd);
    ConnManager& conn_manager = ConnManager::GetInstance();
    conn_manager.RegisterConn(socket_fd, server_conn);
  }
}

int main(int argc, char* argv[]) {
  Socket socket;

  IP server_ip("127.0.0.1");
  PORT server_port(1450); // 0-1024为公共端口,需要root权限
  std::shared_ptr<CallbackWrapper> listen_callback = std::make_shared<CallbackWrapper>(ListenCallback, nullptr);
  
  // 若无循环,程序跑完进程终结,ps当然查不到进程号
  socket.Listen(server_ip, server_port, listen_callback);
  pid_t pid = getpid();
  printf("pid: %d\n", pid);

  Dispatcher& dispatcher = Dispatcher::GetInstance();
  dispatcher.Start();
  // TODO: 脚本测试并发
  return 0;
}
