/*************************************************************************
  > File Name: cclient.cc
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年04月23日 星期四 15时58分28秒
 ************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <memory>
#include <string>

#include "src/utils.h"
#include "src/types.h"
#include "src/macro.h"

class CClient {
 public:
  CClient(int buf_capacity);
  int Connect(const IP& server_ip, PORT server_port);
  void Start();
  void Close();
  DISALLOW_COPY_AND_ASSIGN(CClient);

 private:
  void Send();
  void Recv();
  void SetAddr(sockaddr_in* addr, const IP& server_ip, PORT server_port);

  // 收到响应后的业务处理逻辑
  void HandleResponse(const std::string& response) { 
    printf("client handle response\n"); 
  }

 private:
  SOCKET socket_fd_;
  int buf_capacity_;
  std::unique_ptr<Buffer> in_buffer_;
};

CClient::CClient(int buf_capacity) : in_buffer_(new Buffer(buf_capacity)), 
                                     buf_capacity_(buf_capacity) {}

int CClient::Connect(const IP& server_ip, PORT server_port) {
  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ == INVALID_SOCKET) {
    printf("create socket failed. error code: %d\n", errno);
    return NETLIB_ERROR;
  }

  sockaddr_in server_addr;
  SetAddr(&server_addr, server_ip, server_port);
  int ret = connect(socket_fd_, (sockaddr*)&server_addr, sizeof(server_addr));
  if (ret == SOCKET_ERROR) {
    printf("connect failed, error code: %d\n", errno);
    Close();
  } else {
    printf("connected to %s:%d\n", server_ip.c_str(), server_port);
  }
  return ret;
}

void CClient::Start() {
  pid_t pid = fork();
  if (pid < 0) {
    printf("fork error!\n");
    return;
  } else if (pid == 0) {
    Recv();
    // 父进程退出不会终止子进程,但子进程退出可通过信号终止父进程,较方便
    // 收到对端close,则本端父子进程当关闭
    // 本端写入socket出错,则应关闭写半端,读半端不影响
    // 综合考虑将Recv()放在子进程,Send()放在父进程
    kill(getppid(), SIGTERM); // server close,给父进程发信号终止
    exit(0); // 返回到父进程
  } else {
    printf("child pid: %d\n", pid);
    Send();
    // 从标准输入获取数据出错,无法写入socket,关闭写半端
    shutdown(socket_fd_, SHUT_WR);
    pause(); // 等待子进程的信号
    return; //不影响子进程
  } 
}

void CClient::Recv() {
  // Recv()只负责往in_buffer_写,至于从中取数据去处理,区分数据需要由业务层实现
  uint8_t recv_buf[MAX_SOCKET_BUF_SIZE];
  while (true) {
    // 读完就返回,所以每次读最多只读MAX_SOCKET_BUF_SIZE字节
    int ret = read(socket_fd_, recv_buf, MAX_SOCKET_BUF_SIZE);
    if (ret == SOCKET_ERROR || ret == 0)
      break;
    in_buffer_->Write(recv_buf, ret);

    int content_len = in_buffer_->get_offset();
    if (content_len == 0)
      return;

    uint8_t response_buf[content_len + 1];
    memset(response_buf, 0, sizeof(response_buf));
    ret = in_buffer_->ReadOut(response_buf, content_len);

    std::string response_content((const char*)response_buf);
    printf("response: %s, size: %d\n", response_content.c_str(), (int)response_content.size());

    HandleResponse(response_content);
  }
}

void CClient::Send() {
  // Send()要保证把一条消息尽可能的发送完,因此需要while循环来写,直到写完或者
  // 遇见错误
  // 外层while循环的目的是持续监控stdin,碰到fgets读取错误则退出,由于fgets会
  // 阻塞,因此当没有数据输入时会挂起并不消耗cpu资源
  // TODO: 标准输入输出
  // TODO: 如何在父子进程正确退出(server close能正确反应吗)
  while (true) {
    char request[1024];
    // 表现出来这里的打印内容与子进程的打印内容一起出现,似乎是子进程打印的,
    // 其实不是,是再次while循环执行到该语句导致的
    printf("please send request:\n");
    // 为什么不用char* gets(char* str)?
    // 因为不指定读取的大小,可能导致数组越界
    // 为什么不用scanf或fscanf?
    // 1) 这两者也不能指定读取大小 2) 读取前后都是字符串,不用format转化
    //return; // test SHUTDOWN
    //
    //外层while循环和fgets构成了与用户交互从用户处获取请求的逻辑
    char* result = fgets(request, sizeof(request), stdin);
    if (!result)
      return;
    int offset = 0;
    // 不发送\n
    int remain = (int)strlen(request) - 1;
    // 发送完毕或写入socket失败则返回
    while (remain > 0) {
      int send_size = remain;
      if (send_size > MAX_SOCKET_BUF_SIZE)
        send_size = MAX_SOCKET_BUF_SIZE;
      int ret = write(socket_fd_, request + offset, send_size);
      if (ret < 0)
        break;
      offset += ret;
      remain -= ret;
    }
    if (remain > 0) {
      printf("not all sent\n");
    } else {
      printf("all sent, request: %s, size: %d\n", request, offset);
    } 
  }
}

void CClient::Close() {
  close(socket_fd_);
  printf("socket: %d closed.\n", socket_fd_);
}

void CClient::SetAddr(sockaddr_in* addr, const IP& server_ip, 
    PORT server_port) {
  // 只声明未定义,addr指向的内存数据是不确定的,需要清空后赋值
	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(server_port);
	addr->sin_addr.s_addr = inet_addr(server_ip.c_str());
	if (addr->sin_addr.s_addr == INADDR_NONE)
    printf("gethostbyname failed, ip=%s\n", server_ip.c_str());
}

int main(int argc, char* argv[]) {
  CClient client(16);

  IP server_ip = "127.0.0.1";
  PORT server_port = 1450;
  int ret = client.Connect(server_ip, server_port);

  pid_t pid = getpid();
  printf("pid: %d\n", pid);

  if (ret != NETLIB_ERROR) {
    client.Start();
    client.Close();
  }

  return 0;
}
