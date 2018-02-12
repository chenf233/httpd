#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int main(int argc, char **argv) {
  int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in addr, c_addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(3000);
  bind(sock, (const sockaddr *)&addr, sizeof(addr));

  int vv = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &vv, sizeof(vv));
  setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &vv, sizeof(vv));

  listen(sock, 5);

  int epollfd = kqueue();
  struct kevent *ev = (struct kevent *)malloc(sizeof(struct kevent) * 1024);
  int n = 0;

  EV_SET(&ev[n++], sock, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (void *)(intptr_t)sock);
  kevent(epollfd, ev, n, NULL, 0, NULL);

  while (1) {
    struct kevent *active_events = (struct kevent *)malloc(sizeof(struct kevent) * 1024);
    int n = kevent(epollfd, NULL, 0, active_events, 1024, NULL);
    for (int i = 0; i < n; ++i) {
      int fd = (int)(intptr_t)active_events[i].udata;
      int events = active_events[i].flags;
      std::cout << "event id: " << events << '\n';
      if (events & EV_EOF) {
        std::cout << "disconnected\n";
      } else if (fd == sock) {  // listen socket
        struct sockaddr_in c_addr;
        socklen_t c_addr_len = sizeof(c_addr);
        int c_sock = accept(sock, (sockaddr *)&c_addr, &c_addr_len);
        std::cout << "sock #" << fd << " accept\n";
        EV_SET(&ev[n++], c_sock, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (void *)(intptr_t)c_sock);
        if (kevent(epollfd, ev, n, NULL, 0, NULL) == -1) {
          perror("bad kevent");
          return -1;
        }
      } else if (events & EVFILT_READ) {  // client socket
        std::cout << "sock #" << fd << " read\n";
        char buffer[1024];
        recv(fd, buffer, 1024, 0);
        std::cout << buffer << '\n';
        //break;
      }
    }
  }

  /*
  constexpr int kMaxEvents = 20;
  struct kevent activeEvents[kMaxEvents];
  struct timespec timeout;
  timeout.tv_sec = 3;
  timeout.tv_nsec = 0;
  int n = kevent(epollfd, NULL, 0, activeEvents, kMaxEvents, &timeout);

  for (int i = 0; i < n; ++i) {
    int fd = (int)(intptr_t)activeEvents[i].udata;
    if (fd == sock)
      std::cout << "listen sock\n";
    int events = activeEvents[i].filter;
    if (events == EVFILT_READ)
      std::cout << "fd - " << i << " Ready to read\n";
  }
  */

  /*
  socklen_t c_addr_len = sizeof(c_addr);
  int c_sock = accept(sock, (sockaddr *)&c_addr, &c_addr_len);

  char buffer[1024];
  size_t n = 0;

  struct pollfd pfd;
  pfd.fd = c_sock;
  pfd.events = POLLIN;

  switch (poll(&pfd, 1, 1000)) {
  case -1:
    std::cerr << "error\n";
    return -1;
  case 0:
    std::cout << "timeout\n";
    return -1;
  default:
    n = recv(c_sock, buffer, 1024, 0);
    std::cout << buffer << '\n';
  }
  */

  //close(c_sock);
  close(sock);
  return 0;
}