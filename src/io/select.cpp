#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv) {
  fd_set fdset;
  FD_ZERO(&fdset);

  int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in addr, c_addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(3000);
  bind(sock, (const sockaddr *)&addr, sizeof(addr));

  listen(sock, 5);
  socklen_t c_addr_len = sizeof(c_addr);
  int c_sock = accept(sock, (sockaddr *)&c_addr, &c_addr_len);

  char buffer[1024];
  size_t n = 0;
  FD_SET(c_sock, &fdset);
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  switch (select(c_sock+1, &fdset, NULL, NULL, &tv)) {
  case -1:
    return -1;
  case 0:
    std::cout << "timeout\n";
    return -1;
  default:
    std::cout << "about to read\n";
    n = recv(c_sock, buffer, 1024, 0);
    buffer[n] = '\n';
    std::cout << buffer << '\n';
  }

  close(c_sock);
  close(sock);
  return 0;
}