/*************************************************************************
  > File Name: client.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月23日 星期四 14时51分23秒
 ************************************************************************/

#include <stdio.h>
#include <src/types.h>
#include <string.h>
#include <memory>

#include "src/socket.h"
#include "src/dispatcher.h"
#include "src/types.h"
#include "src/macro.h"
#include "src/utils.h"
#include "src/conn.h"
#include "src/conn_manager.h"

class ClientConn : public Conn {
 public:
  ClientConn(int buf_capacity, SOCKET socket_fd) : 
    Conn(buf_capacity, socket_fd) {}
  void OnRead() override;
  void OnClose() override;
  DISALLOW_COPY_AND_ASSIGN(ClientConn);

 private:
  void HandleRequest() override { 
    printf("sclient handle request, maybe transfer request\n"); 
  }
  void HandleResponse() override { 
      printf("sclient handle response, maybe transfer response\n"); 
  }
};

void ClientConn::OnRead() {
  std::shared_ptr<Socket> socket = GetSocket(socket_fd_);
  if (!socket)
    return;
  char temp[MAX_SOCKET_BUF_SIZE];
  while (true) {
    memset(temp, 0, MAX_SOCKET_BUF_SIZE);
    // ret不一定等于期望读取的字节数
    int ret = socket->Recv(temp, MAX_SOCKET_BUF_SIZE);
    if (ret < 0)
      break;
    in_buffer_->Write(temp, ret);
  }
  // printf遇'\0'停止,对于存储在in_buffer_中多个以'\0'结尾的字符串,只能
  // 读出一个
  printf("recv: %s\n", in_buffer_->get_buffer());
  printf("recv size: %d\n", in_buffer_->get_offset());
  
  MSG_SOURCE source = FROM_SERVER;
  switch (source) {
    case FROM_CLIENT: 
      HandleRequest();
      break;
    case FROM_SERVER:
      HandleResponse();
      break;
    default:
      break;
  }
}

void ClientConn::OnClose() {
  ConnManager& conn_manager = ConnManager::GetInstance();
  if (conn_manager.FindConn(socket_fd_))
    conn_manager.UnregisterConn(socket_fd_);

  std::shared_ptr<Socket> socket = GetSocket(socket_fd_);
  if (socket)
    socket->Close();
}


void ConnCallback(void* callback_data, SOCKET socket_fd, MSG_TYPE type) {
  ConnManager& conn_manager = ConnManager::GetInstance();
  std::shared_ptr<Conn> client_conn = conn_manager.FindConn(socket_fd);
  if (!client_conn)
    return;

  switch (type) {
    //    // DO: printf乱码问题,缺少'\0'导致
    //    // TODO: 客户端交互使用阻塞socket/select/epoll的实现?
    //    // TODO: 业务逻辑,工作者线程池
    //    // TODO: 定时事件?
    //    // TODO: deamon进程提供服务
    //    // TODO: 简单的rpc调用
    case NETLIB_MSG_READ:
      // 接受数据的逻辑
      {
        //printf("client prepare to read.\n");
        client_conn->OnRead();
      }
      break;
    case NETLIB_MSG_WRITE:
      // 发送数据的逻辑
      {
        //printf("client prepare to write.\n");
        client_conn->OnWrite();
      }
      break;
    case NETLIB_MSG_CLOSE:
      // 关闭socket前的逻辑
      {
        printf("client prepare to close.\n");
        client_conn->OnClose(); 
      }
      break;
    default:
      break;
  }
}

void ConnectCallback(void* callback_data, SOCKET socket_fd, MSG_TYPE type) {
  if (type == NETLIB_MSG_CONFIRM) {
    // 需要在函数中将一个对象的指针赋值给别人,则该对象只能定义在堆上
    // 若定义在栈上,离开函数对象清楚,指针失效
    std::shared_ptr<Socket> conn_socket = GetSocket(socket_fd);
    std::shared_ptr<CallbackWrapper> conn_callback = std::make_shared<CallbackWrapper>(ConnCallback, nullptr);
    conn_socket->set_callback(conn_callback);
    
    // 直到建立连接后client才能发送消息,因此使用回调函数,条件发生时调用
    // 建立连接时需要: 1) send 2) 应对可能的close
    // 建立连接后需要: 1) 应对read 2) 应对write 3) 应对close
    // 因此需要另外一个回调函数负责建立连接后的动作
    // 两个函数无法直接通信(不能通过传参的方式),但都需要调用ClientConn对象,因此
    // 将ClientConn对象存储在一个map中,通过key: socket_fd存取
    // 最好不要使用全局变量,因此将该map封装在ConnManager类中
    std::shared_ptr<ClientConn> client_conn = std::make_shared<ClientConn>(10, socket_fd);
    ConnManager& conn_manager = ConnManager::GetInstance();
    conn_manager.RegisterConn(socket_fd, client_conn);
    printf("client connected to server.\n");

    // connect成功后发送request不必须
    char request[2048] = "zzzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozzhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguozhongguohongguo";
    printf("request len: %d\n", (int)strlen(request)+1);
    client_conn->Send(request, strlen(request)+1);  
  } else if (type == NETLIB_MSG_CLOSE) {
      std::shared_ptr<Socket> socket = GetSocket(socket_fd);
      if (socket)
        socket->Close();   
  }
}

int main(int argc, char* argv[]) {
  // 由于Socket对象使用shared_ptr存储,如果在栈上创建,shared_ptr释放资源时把
  // 该对象释放掉了,程序结束时(如ctrl+c),原来的栈对象二次释放,出bug
  // server为什么可以?
  // server在栈上定义的是listen socket,连接释放的是conn socket,不影响listen socket
  // 如果一个对象使用智能指针管理了,一定不能在栈上定义
  std::unique_ptr<Socket> socket(new Socket());

  IP server_ip("127.0.0.1");
  PORT server_port(1450); // 0-1024为公共端口,需要root权限
  std::shared_ptr<CallbackWrapper> connect_callback = std::make_shared<CallbackWrapper>(ConnectCallback, nullptr);
  
  // 若无循环,程序跑完进程终结,ps当然查不到进程号
  socket->Connect(server_ip, server_port, connect_callback);
  pid_t pid = getpid();
  printf("pid: %d\n", pid);

  // client和server都使用dispatcher来处理socket读写,不浪费吗?
  // 这里的server与client是一体的,且其server角色为主,client角色为辅,
  // 可作为client向其他server发起请求
  // 最初所有的请求都来源于用户(真正的client),这里的server和client混合体向
  // 其他server发起的请求也是被动出现的
  Dispatcher& dispatcher = Dispatcher::GetInstance();
  dispatcher.Start();

  return 0;
}
