#ifndef UDHO_MIDDLEWARE_H
#define UDHO_MIDDLEWARE_H

#include <set>
#include <deque>
#include <mutex>
#include <chrono>

namespace udho {
namespace middleware {

// struct ip_filter{
//     enum class mode{
//         permissive,
//         restrictive
//     };

//     inline explicit ip_filter(mode m): _mode(m) {}
//     bool operator()(const boost::asio::ip::address& address) const {
//         std::lock_guard<std::mutex> lock(_mutex);
//         if(_allowed.count(address) > 0) {
//             return true;
//         } else {
//             if(restrictive()) {
//                 return false;
//             } else {
//                 return _denied.count(address) == 0;
//             }
//         }
//     }

//     inline bool permissive()  const { return _mode == mode::permissive; }
//     inline bool restrictive() const { return _mode == mode::restrictive; }

//     inline void allow(const boost::asio::ip::address& address) {
//         std::lock_guard<std::mutex> lock(_mutex);
//         _allowed.insert(address);
//         _denied.erase(address);
//     }

//     inline void deny(const boost::asio::ip::address& address) {
//         std::lock_guard<std::mutex> lock(_mutex);
//         _denied.insert(address);
//         _allowed.erase(address);
//     }

//     private:
//         mutable std::mutex _mutex;
//         mode _mode;
//         std::set<boost::asio::ip::address> _allowed, _denied;
// };

// struct rate_limiter{
//     using time_type = std::deque<std::chrono::steady_clock::time_point>;
//     using duration_type = std::chrono::minutes;

//     inline rate_limiter(std::chrono::minutes window, std::size_t capacity): _window(window), _capacity(capacity) {}

//     inline bool operator()(const boost::asio::ip::address& address, const udho::net::types::headers::request& request){
//         std::lock_guard<std::mutex> lock(_mutex);

//         auto now = std::chrono::steady_clock::now();
//         _dequeue.push_front(now);

//         if(_dequeue.size() < _capacity) {
//             return true;
//         } else {
//             while(now - _dequeue.back() > _window) {
//                 _dequeue.pop_front();
//             }

//             return _dequeue.size() <= _capacity;
//         }
//     }

//     private:
//         std::mutex _mutex;
//         time_type _dequeue;
//         duration_type _window;
//         std::size_t _capacity;
// };




}
}

#endif // UDHO_MIDDLEWARE_H
