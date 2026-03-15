#ifndef VKRT_DELETIONQUEUE_HPP
#define VKRT_DELETIONQUEUE_HPP

#include <queue>

namespace vkrt {

struct DeletionQueue {
private:
    std::queue<std::function<void()>> _deletionQueue;
public:
    void flushQueue() {
        for (; !_deletionQueue.empty(); _deletionQueue.pop()) {
            (_deletionQueue.front())();
        }
    }
    void pushFunction(std::function<void()> function) { _deletionQueue.push(function); }
};

} // namespace vkrt

#endif // VKRT_DELETIONQUEUE_HPP