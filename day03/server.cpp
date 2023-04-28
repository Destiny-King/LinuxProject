#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include "util.h"

#define MAX_EVENTS 1024
#define READ_BUFFER 1024

void setnonblocking(int fd)
{
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

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

  int epfd = epoll_create1(0);
  errif(epfd == -1, "epoll create error");

  struct epoll_event events[MAX_EVENTS], ev;
  bzero(&events, sizeof(events));

  bzero(&ev, sizeof(ev));
  ev.data.fd = sockfd;
  ev.events = EPOLLIN | EPOLLET;
  setnonblocking(sockfd);
  epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

  // 不断监听epoll上的事件并处理
  while (true)
  {
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < nfds; i++)
    {
      if (events[i].data.fd == sockfd)
      {
        // 接收客户端连接
        struct sockaddr_in clnt_addr;
        bzero(&clnt_addr, sizeof(clnt_addr));
        socklen_t clnt_addr_len = sizeof(clnt_addr);

        int clnt_sockfd = accept(sockfd, (sockaddr *)&clnt_addr, &clnt_addr_len);
        errif(clnt_sockfd == -1, "socket accept error");

        printf("new client fd %d! IP: %s Port: %d\n", clnt_sockfd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

        bzero(&ev, sizeof(ev));
        ev.data.fd = clnt_sockfd;
        ev.events = EPOLLIN | EPOLLET;
        setnonblocking(clnt_sockfd);
        epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sockfd, &ev);
      }
      else if (events[i].events & EPOLLIN)
      {
        char buf[READ_BUFFER];
        // 读取
        while (true)
        {                                                                 // 定义缓冲区
          bzero(&buf, sizeof(buf));                                       // 清空缓冲区
          ssize_t read_bytes = read(events[i].data.fd, buf, sizeof(buf)); // 读取数据
          if (read_bytes > 0)
          {
            printf("message from client fd %d: %s\n", events[i].data.fd, buf);
            write(events[i].data.fd, buf, sizeof(buf)); // 写回客户端
          }
          else if (read_bytes == 0) // EOF
          {
            printf("client fd %d disconnected\n", events[i].data.fd);
            close(events[i].data.fd);
            break;
          }
          else if (read_bytes == -1 && errno == EINTR) // 发生错误
          {
            printf("continue reading");
            continue;
          }
          else if (read_bytes == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
          {
            printf("finish reading once, errno: %d\n", errno);
            break;
          }
        }
      }
      else
      {
        printf("something else happened\n");
      }
    }
  }

  close(sockfd);

  return 0;
}