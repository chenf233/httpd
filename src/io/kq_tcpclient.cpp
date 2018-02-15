#include <arpa/inet.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

constexpr size_t kBufSize = 1024;

void diep(const char *s) {
  perror(s);
  exit(EXIT_FAILURE);
}

int tcpopen(const char *host, uint16_t port) {
  auto hp = gethostbyname(host);
  if (hp == nullptr)
    diep("gethostbyname()");

  auto sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0)
    diep("socket()");

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  auto addr_it = (in_addr **)hp->h_addr_list;
  while (1) {
    if (*addr_it == nullptr)
      diep("connect()");

    addr.sin_addr = **addr_it;
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == 0)
      break;
    ++addr_it;
  }

  return sockfd;
}

void sendbuf(int sockfd, const char *buf, int len) {
  int bytes_sent, pos;
  pos = 0;
  do {
    if ((bytes_sent = send(sockfd, buf+pos, len-pos, 0)) < 0)
      diep("send()");
    pos += bytes_sent;
  } while (bytes_sent > 0);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s host port\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  auto sockfd = tcpopen(argv[1], atoi(argv[2]));

  struct kevent chlist[2], evlist[2];
  auto kq = kqueue();
  if (kq == -1)
    diep("kqueue()");

  EV_SET(&chlist[0], sockfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
  EV_SET(&chlist[1], fileno(stdin), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);

  char buf[kBufSize];
  while (1) {
    auto nev = kevent(kq, chlist, 2, evlist, 2, nullptr);
    if (nev < 0)
      diep("kquque()");
    else if (nev > 0) {
      if (evlist[0].flags & EV_EOF)  // read direction of socket has shutdown
        exit(EXIT_FAILURE);

      for (auto i = 0; i < nev; i++) {
        if (evlist[i].flags & EV_ERROR) {
          fprintf(stderr, "EV_ERROR: %s\n", strerror(evlist[i].data));
          exit(EXIT_FAILURE);
        }

        if (evlist[i].ident == sockfd) {
          memset(buf, 0, kBufSize);
          if (read(sockfd, buf, kBufSize) < 0)
            diep("read()");
          fputs(buf, stdout);
        } else if (evlist[i].ident == fileno(stdin)) {
          memset(buf, 0, kBufSize);
          fgets(buf, kBufSize, stdin);
          sendbuf(sockfd, buf, strlen(buf));
        }
      }
    }
  }

  close(kq);
  return 0;
}
