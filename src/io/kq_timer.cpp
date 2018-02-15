#include <sys/event.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // for strerror()
#include <unistd.h>

void diep(const char *s) {
  perror(s);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  int kq = kqueue();
  if (kq == -1)
    diep("kqueue()");

  struct kevent change, event;
  EV_SET(&change, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 5000, 0);

  while (1) {
    int nev = kevent(kq, &change, 1, &event, 1, NULL);
    if (nev < 0) {
      diep("kevent()");
    } else if (nev > 0) {
      if (event.flags & EV_ERROR) {
        fprintf(stderr, "EV_ERROR: %s\n", strerror(event.data));
        exit(EXIT_FAILURE);
      }

      pid_t pid = fork();
      if (pid < 0) {
        diep("fork()");
      } else if (pid == 0) {  // Child process
        if (execlp("date", "date", (char *)0) < 0)
          diep("execlp()");
      }
    }
  }

  close(kq);
  return EXIT_SUCCESS;
}
