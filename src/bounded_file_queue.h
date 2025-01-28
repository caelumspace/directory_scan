#include <queue>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <atomic>

class BoundedFileQueue {
public:
    // Constructor sets the maximum size of the queue (number of file paths to hold).
    explicit BoundedFileQueue(size_t maxSize)
        : maxSize_(maxSize), finished_(false) {}

    // Producer: push a path into the queue if there's room, or wait until there is.
    void push(const std::filesystem::path& path) {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait until there's space or we're finished
        condProducer_.wait(lock, [this]() {
            return queue_.size() < maxSize_ || finished_;
        });

        if (finished_) {
            // If finished_ is set, we don't enqueue further items
            return;
        }

        queue_.push(path);

        // Notify a consumer that there is now at least one item
        condConsumer_.notify_one();
    }

    // Consumer: pop a path from the queue if available, or wait until there's one.
    // Returns false if the queue is empty *and* the queue is finished (no more items).
    bool pop(std::filesystem::path& path) {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait until we have something to pop or we are finished
        condConsumer_.wait(lock, [this]() {
            return !queue_.empty() || finished_;
        });

        // If empty and finished, no more items will ever arrive
        if (queue_.empty()) {
            return false;
        }

        path = queue_.front();
        queue_.pop();

        // Notify the producer that there is space now
        condProducer_.notify_one();
        return true;
    }

    // Signals no more items will be produced
    void setFinished() {
        std::lock_guard<std::mutex> lock(mutex_);
        finished_ = true;

        // Wake up any waiting producer and consumer
        condProducer_.notify_all();
        condConsumer_.notify_all();
    }

    bool isFinished() 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return finished_;
    }

    bool isEmpty() 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }






private:
    std::queue<std::filesystem::path> queue_;
    std::mutex mutex_;
    std::condition_variable condProducer_;
    std::condition_variable condConsumer_;
    const size_t maxSize_;

    bool finished_;
};
