#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <poll.h>

int main(int argc, char **argv) {
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

  close(c_sock);
  close(sock);
  return 0;
}