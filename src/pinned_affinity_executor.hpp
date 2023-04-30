#ifndef PINNED_AFFINITY_EXECUTOR_HPP
#define PINNED_AFFINITY_EXECUTOR_HPP

#include <atomic>
#include <thread>
#include <vector>
#include <functional>

class PinnedAffinityExecutor {
public:
    PinnedAffinityExecutor(unsigned n_workers = std::thread::hardware_concurrency());
    ~PinnedAffinityExecutor();
    void Invoke(std::function<void(unsigned)>&& f);
    inline unsigned NumberOfWorkers() { return threads_.size(); }

    template<typename T>
    std::tuple<T, T> SplitTask(T n_tasks, unsigned worker_index) {
        auto task_size = (n_tasks + (NumberOfWorkers() - 1)) / NumberOfWorkers();
        return std::make_tuple(task_size * worker_index,
                               std::min(n_tasks, task_size * (worker_index + 1)));
    }

private:
    typedef std::atomic_uint64_t atomic_unsigned_type;
    static constexpr int atomic_unsigned_type_bits = std::numeric_limits<atomic_unsigned_type::value_type>::digits;

    void WorkerThread(unsigned);

    std::vector<std::thread> threads_;
    std::function<void(unsigned)> f_;
    std::atomic_int32_t signal_;
    std::atomic_flag shutdown_;
    std::vector<atomic_unsigned_type> bitmap_;
};

#endif
