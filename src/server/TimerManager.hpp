#pragma once
#include <chrono>
#include <functional>
#include <vector>
#include <queue>
#include <unordered_map>
#include <cstdint>

namespace server {

using TimePoint = std::chrono::steady_clock::time_point;

struct TimerEvent {
    TimePoint expiration;
    uint64_t timer_id;
    int fd;
    
    // Min-heap ordering: lowest expiration time comes first
    bool operator>(const TimerEvent& other) const {
        return expiration > other.expiration;
    }
};

class TimerManager {
public:
    TimerManager();

    uint64_t add_timer(int fd, std::chrono::milliseconds timeout);
    void cancel_timer(uint64_t timer_id);

    // Returns how many milliseconds until the next timer expires, or -1 if empty
    int get_next_timeout() const;

    // Process all expired timers
    void handle_expired_timers(std::function<void(int)> on_timeout);

private:
    std::priority_queue<TimerEvent, std::vector<TimerEvent>, std::greater<TimerEvent>> timers_;
    std::unordered_map<uint64_t, bool> active_timers_;
    uint64_t next_timer_id_{0};
};

} // namespace server
