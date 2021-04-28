#include <mutex>
#include <optional>
#include <queue>
#include <string>

namespace echelon
{
    struct JobItem
    {
	    mutable long Id;
        std::string DataType;
        std::string Data;
        float TickTime;
        std::string GameTime;
    };
	
    class JobQueue {
        std::queue<JobItem> queue_;
        mutable std::mutex mutex_;

        // Moved out of public interface to prevent races between this
        // and pop().
        bool empty() const {
            return queue_.empty();
        }

        long id = 0;

    public:
        JobQueue() = default;
        JobQueue(const JobQueue&) = delete;
        JobQueue& operator=(const JobQueue&) = delete;

        JobQueue(JobQueue&& other) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_ = std::move(other.queue_);
        }

        virtual ~JobQueue() { }

        unsigned long size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        std::optional<JobItem> pop() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) {
                return {};
            }
            auto tmp = queue_.front();
            queue_.pop();
            return tmp;
        }

        void push(const JobItem& item) {
            std::lock_guard<std::mutex> lock(mutex_);
            item.Id = ++id;
            queue_.push(item);
        }
    };
}
