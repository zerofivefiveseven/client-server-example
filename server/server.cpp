#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#define PORT 8080
#define SOCK_PATH

int main(void) {
  int s, s2, t, len;
  sockaddr_un local, remote;
  char str[129];
  str[128] = '\0';

  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, "echo_socket");
  unlink(local.sun_path);
  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(s, (struct sockaddr *) &local, len) == -1) {
    perror("bind");
    exit(1);
  }

  if (listen(s, 5) == -1) {
    perror("listen");
    exit(1);
  }

  for (;;) {
    int done, n = 0;
    printf("Waiting for a connection...\n");
    t = sizeof(remote);
    if ((s2 = accept(s, (struct sockaddr *) &remote, reinterpret_cast<socklen_t *>(&t))) == -1) {
      perror("accept");
      exit(1);
    }

    printf("Connected.\n");

    done = 0;
    do {
      n += recv(s2, str + n, 128 - n, 0);
    } while (n != 128);
    if (strlen(str) > 2 && std::stoi(str) % 32 == 0){
      printf("Data received.\n");
    } else {
      printf("Data error.\n");
    }
    n = 0;

    std::cout << str<<std::endl;
    close(s2);
  }

  return 0;
}
