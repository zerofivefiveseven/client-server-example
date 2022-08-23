#include <iostream>
#include <thread>
#include <algorithm>
#include <cassert>
#include <string>
#include <mutex>
#include <condition_variable>
#include <numeric>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <variant>
#define SOCK_PATH

class ClientSocket {
  explicit ClientSocket(int fd) : fd(fd) {}
 public:

  enum ErrorCode {
    SOCKET_CREATE_ERROR,
    SOCKET_CONNECT_ERROR
  };

  static std::variant<ClientSocket, ErrorCode> createClientSocket() {
    int fd;
    size_t len;
    sockaddr_un remote{};

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      return SOCKET_CREATE_ERROR;
    }

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, "echo_socket");
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(fd, (struct sockaddr *) &remote, len) == -1) {
      return SOCKET_CONNECT_ERROR;
    }

    return ClientSocket(fd);
  }

  template<class T>
  void send(T sum) const {
    char str[128];
    std::fill(str, str + 128, 0);
    auto string_sum = std::to_string(sum);
    std::copy(string_sum.begin(), string_sum.end(), str);
    if (::send(fd, str, 128, 0) == -1) {
      perror("send");
    }
  }

  void close() const {
    ::close(fd);
  }

 private:
  int fd;
};

std::size_t replace_all(std::string &inout, std::string_view what, std::string_view with) {
  std::size_t count{};
  for (std::string::size_type pos{};
       std::string::npos != (pos = inout.find(what.data(), pos, what.length()));
       pos += with.length(), ++count) {
    inout.replace(pos, what.length(), with.data(), with.length());
  }
  return count;
}

int main() {
  std::mutex mutex;hgfhgfjjjjghfjfghj
  std::condition_variable cv;
  std::string data;
  bool ready = false;
  bool processed = false;

  char BUFFER[128];

  std::thread thread(
      [&]() {
        while (true) {
          std::cerr << "Trying to connect...\n";
          auto maybeSocket = ClientSocket::createClientSocket();

          if (std::holds_alternative<ClientSocket::ErrorCode>(maybeSocket)) {
            auto error_code = std::get<ClientSocket::ErrorCode>(maybeSocket);
            if (error_code == ClientSocket::SOCKET_CONNECT_ERROR) {
              sleep(2);
              continue;
            } else {
              perror("Create socket");
              throw std::runtime_error("Cannot create socket");
            }
          }
          auto socket = std::get<ClientSocket>(maybeSocket);
          std::cerr << "Connected.\n";

          std::unique_lock lk(mutex);
          cv.wait(lk, [&] { return ready; });

          std::string data = BUFFER;
          std::fill(BUFFER, BUFFER + 128, 0);

          std::cout << data << '\n';

          size_t sum = std::accumulate(data.begin(), data.end(), 0, [](size_t acc, char c) {
            if (!isdigit(c)) return acc;
            return acc + c - '0';
          });
          std::cout << sum << '\n';

          socket.send(sum);

          processed = true;
          ready = false;
          lk.unlock();
          cv.notify_one();
          socket.close();
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

    std::copy(input.begin(), input.end(), BUFFER);

    {
      std::lock_guard lk(mutex);
      ready = true;
    }
    cv.notify_one();
    {
      std::unique_lock lk(mutex);
      cv.wait(lk, [&] { return processed; });
      processed = false;
    }
  }
  return 0;
}
