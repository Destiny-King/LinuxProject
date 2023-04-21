#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "util.h"

int main()
{
  // 初始化套接字
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  errif(sockfd == -1, "socket create error");

  // 初始化结构体
  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));

  // 设置地址族、IP和端口
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_port = htons(8888);

  // 绑定
  errif(bind(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1, "socket bind error");

  // 监听
  errif(listen(sockfd, SOMAXCONN) == -1, "socket listen error");

  // 接收客户端连接
  struct sockaddr_in clnt_addr;
  socklen_t clnt_addr_len = sizeof(clnt_addr);
  bzero(&clnt_addr, sizeof(clnt_addr));
  int clnt_sockfd = accept(sockfd, (sockaddr *)&clnt_addr, &clnt_addr_len);
  errif(clnt_sockfd == -1, "socket accept error");

  printf("new client fd %d! IP: %s Port: %d\n", clnt_sockfd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

  // 读取
  while (true)
  {
    char buf[1024];                                           // 定义缓冲区
    bzero(&buf, sizeof(buf));                                 // 清空缓冲区
    ssize_t read_bytes = read(clnt_sockfd, buf, sizeof(buf)); // 读取数据
    if (read_bytes > 0)
    {
      printf("message from client fd %d: %s\n", clnt_sockfd, buf);
      write(clnt_sockfd, buf, sizeof(buf)); // 写回客户端
    }
    else if (read_bytes == 0) // EOF
    {
      printf("client fd %d disconnected\n", clnt_sockfd);
      close(clnt_sockfd);
      break;
    }
    else if (read_bytes == -1) // 发生错误
    {
      close(clnt_sockfd);
      errif(true, "socket read error");
    }
  }
  close(sockfd);

  return 0;
}