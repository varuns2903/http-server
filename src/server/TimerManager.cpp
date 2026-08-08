#include "TimerManager.hpp"

namespace server {

TimerManager::TimerManager() = default;

uint64_t TimerManager::add_timer(int fd, std::chrono::milliseconds timeout) {
    uint64_t id = ++next_timer_id_;
    TimePoint expiration = std::chrono::steady_clock::now() + timeout;
    
    timers_.push({expiration, id, fd});
    active_timers_[id] = true;
    
    return id;
}

void TimerManager::cancel_timer(uint64_t timer_id) {
    // Lazy deletion: just mark it inactive
    auto it = active_timers_.find(timer_id);
    if (it != active_timers_.end()) {
        it->second = false;
    }
}

int TimerManager::get_next_timeout() const {
    if (timers_.empty()) {
        return -1; // Infinite timeout
    }
    
    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(timers_.top().expiration - now).count();
    
    if (diff < 0) return 0; // Already expired! Wake up immediately
    return static_cast<int>(diff);
}

void TimerManager::handle_expired_timers(std::function<void(int)> on_timeout) {
    auto now = std::chrono::steady_clock::now();
    
    while (!timers_.empty()) {
        auto top = timers_.top();
        
        if (top.expiration > now) {
            break; // No more expired timers
        }
        
        timers_.pop();
        
        if (active_timers_[top.timer_id]) {
            active_timers_.erase(top.timer_id);
            on_timeout(top.fd);
        } else {
            // It was canceled lazily, just erase and ignore
            active_timers_.erase(top.timer_id);
        }
    }
}

} // namespace server
