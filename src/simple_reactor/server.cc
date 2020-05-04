/*************************************************************************
  > File Name: main.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月22日 星期三 18时55分15秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <memory>
#include <string>

#include "src/simple_reactor/socket.h"
#include "src/simple_reactor/dispatcher.h"
#include "src/types.h"
#include "src/utils.h"
#include "src/macro.h"
#include "src/simple_reactor/conn.h"
#include "src/simple_reactor/conn_manager.h"

class ServerConn : public Conn {
 public:
  ServerConn(int buf_capacity, SOCKET socket_fd): 
     Conn(buf_capacity, socket_fd) {}
  void OnRead() override;
  void OnClose() override;
  DISALLOW_COPY_AND_ASSIGN(ServerConn);

 private:
  void HandleRequest(const std::string& request) override { 
    printf("server handle request\n"); 
  }
  void HandleResponse(const std::string& response) override {}
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
  // 当只保留打印in_buffer_的printf时,epoll LT和while循环读随机出莫名其妙的错误:
  // 增加ret的printf后该错误消失,似乎printf会影响程序的运行时表现,太可怕了!
  //
  // 明明client只send了一次,而server已经完全接收进in_buffer_,
  // 但第二次触发OnRead()打印in_buffer_,内容居然是第一次的截短
  // 后的后半部分;
  // 从cpu使用率来看,LT一直在占用cpu,频繁的触发onread等条件,
  // 按理说应该会一直打印才对,就像在server_callback()中的printf
  // 一样,但奇怪的是并没有一直打印,而只打印2次
  // accept conn, socket fd=5 from 127.0.0.1:44550
  // server allowed connection.
  // recv: hello, server!
  // all sent: nihao, client
  // recv: erver!
  // all sent: nihao, client
  int content_len = in_buffer_->get_offset();
  if (content_len == 0)
    return;

  uint8_t request_buf[content_len + 1];
  memset(request_buf, 0, sizeof(request_buf));
  int ret = in_buffer_->ReadOut(request_buf, content_len);

  std::string request_content((const char*)request_buf);
  printf("request: %s, size: %d\n", request_content.c_str(), (int)request_content.size());

  HandleRequest(request_content);
  
  //int expect_response_size = 40;
  //char response[expect_response_size];
  //int ret = in_buffer_->ReadOut(response, expect_response_size);
  char response[20] = "nihao, client";
  // 发端发多少,手端收多少
  // 二者需要约定一条消息的格式,以识别不同消息的边界,因为TCP是基于数据流的,
  // 组装包(构建消息)和拆包(解析消息)都在应用层的业务逻辑中实现
  // 从数据流的形式来看,char与unsigned char(bytes)并无区别,'\0'也是一个字节
  // 只是纯char数组用内置的c字符串函数不好处理,比如您发了一连串的小包,均以
  // '\0'结尾,收端收到后全存在缓冲区,你怎么用语言提供的字符串处理函数拆包?
  // c和c++对字符串的分割这么基本的功能都没有提供函数处理
  // 对web应用,消息基本都是字符串,编程起来用c++该有多麻烦
  // 虽然字符串的拆包也需要开发者自己来完成,但c++提供的函数实在是不够用
  // 没有字符串类型,没有utf-8编码支持,std::string也没有提供方便的函数
  // char数组更是还要分配固定大小的空间来应对可能长短不一的字符串
  // 还要担心越界
  printf("response: %s, len: %d\n", response, (int)strlen(response));
  Send((uint8_t*)response, (int)strlen(response));
  // get data from in_buffer_, service handle, generate response, Send(response, len_of_response)
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

  return 0;
}
