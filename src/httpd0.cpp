#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

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
  int conn_sock = accept(sock, (sockaddr *)&c_addr, &c_addr_len);

  char buffer[1024];
  ssize_t n = recv(conn_sock, buffer, 1024, 0);
  buffer[n] = '\0';
  std::cout << buffer;

  const char *message = "It works!";
  std::stringstream ss;
  ss << "HTTP/1.1 200 OK\n"
     << "Content-Length: " << strlen(message) << "\r\n"
     << "\r\n"
     << message;

  send(conn_sock, ss.str().data(), ss.str().size(), 0);

  close(conn_sock);
  close(sock);

  return 0;
}