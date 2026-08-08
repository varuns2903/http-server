#pragma once
#include <sys/epoll.h>
#include <vector>

namespace network {

class Epoll {
public:
    Epoll();
    ~Epoll();

    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;

    void add(int fd, uint32_t events);
    void modify(int fd, uint32_t events);
    void remove(int fd);

    // Blocks and waits for OS file descriptor events
    int wait(std::vector<epoll_event>& events, int timeout_ms = -1);

private:
    int epoll_fd_;
};

} // namespace network
