#include <iostream>
#include <thread>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <mutex>
#include <condition_variable>
#include <numeric>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#define SOCK_PATH

std::size_t replace_all(std::string &inout, std::string_view what, std::string_view with) {
  std::size_t count{};
  for (std::string::size_type pos{};
       std::string::npos != (pos = inout.find(what.data(), pos, what.length()));
       pos += with.length(), ++count) {
    inout.replace(pos, what.length(), with.data(), with.length());
  }
  return count;
}

std::mutex mutexIntance;
std::condition_variable cv;
std::string data;
bool ready = false;
bool processed = false;

//std::string BUFFER;
char BUFFER[128];

int main() {
//  std::cout <<  std::to_string(0);
//  return 0;


  std::thread thread(
      []() {
        int s, t, len;
        struct sockaddr_un remote;
        char str[128];

        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
          perror("socket");
          exit(1);
        }

        printf("Trying to connect...\n");

        remote.sun_family = AF_UNIX;
        strcpy(remote.sun_path, "echo_socket");
        len = strlen(remote.sun_path) + sizeof(remote.sun_family);
        if (connect(s, (struct sockaddr *)&remote, len) == -1) {
          perror("connect");
          exit(1);
        }


        printf("Connected.\n");

        while (true) {
          std::unique_lock lk(mutexIntance);
          cv.wait(lk, [] { return ready; });

          std::string data = BUFFER;
//          BUFFER = "";
          std::fill(BUFFER, BUFFER + 128, 0);

          std::cout << data << '\n';

          size_t sum = std::accumulate(data.begin(), data.end(), 0, [](size_t acc, char c) {
            if (!isdigit(c)) return acc;
            return acc + c - '0';
          });
          std::cout << sum << '\n';
          auto string_sum = std::to_string(sum);
//          str[0] = sum;
          std::copy(string_sum.begin(), string_sum.end(), str);

//        while(printf("> "), fgets(str, 100, stdin), !feof(stdin)) {
          if (send(s, str,  128, 0) == -1) {
            perror("send");
            exit(1);
          }
          std::fill(str, str + 128, 0);

//        }
//        close(s);
          processed = true;
          ready = false;
          lk.unlock();
          cv.notify_one();
        }
      }
  );

  while (true) {
    std::string input;
    std::cin >> input;
    if (!std::all_of(input.begin(), input.end(), [](char c) { return isdigit(c); })) {
      std::this_thread::yield();
      std::cout << "invalid string\n";
      continue;
    }

    std::sort(input.begin(), input.end(), [](char ch1, char ch2) { return ch1 > ch2; });

    std::replace_if(input.begin(), input.end(), [](char c) { return c & 1; }, 'N');
    replace_all(input, "N", "KB");

//    BUFFER = input;
    std::copy(input.begin(), input.end(), BUFFER);

    {
      std::lock_guard lk(mutexIntance);
      ready = true;
    }
    cv.notify_one();
    {
      std::unique_lock lk(mutexIntance);
      cv.wait(lk, [] { return processed; });
      processed = false;
    }

  }
  return 0;
}
