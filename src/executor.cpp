#include <limits>
#include <numa.h>
#include "pinned_affinity_executor.hpp"


void PinnedAffinityExecutor::WorkerThread(unsigned thread_index) {
    const auto bitmap_index = thread_index / atomic_unsigned_type_bits;
    const auto bitmap_pos = thread_index % atomic_unsigned_type_bits;
    const auto bitmap_bit = static_cast<atomic_unsigned_type::value_type>(1) << bitmap_pos;
    const auto bitmap_mask = ~bitmap_bit;

    numa_run_on_node(thread_index);

    while (!shutdown_.test(std::memory_order_relaxed)) {
        bitmap_[bitmap_index].wait(0);
        const auto bitmap = bitmap_[bitmap_index].load(std::memory_order_acquire) & bitmap_bit;
        if (bitmap == 0) continue;
        f_(thread_index);
        bitmap_[bitmap_index].fetch_and(bitmap_mask, std::memory_order_acq_rel);
        signal_.fetch_add(1, std::memory_order_acq_rel);
        signal_.notify_one();
    }
}

PinnedAffinityExecutor::PinnedAffinityExecutor(unsigned n_workers)
    : threads_(), f_(), signal_(0), shutdown_(false),
      bitmap_((n_workers + atomic_unsigned_type_bits - 1) / atomic_unsigned_type_bits)
{
    threads_.reserve(n_workers);
    for (unsigned i = 0; i < n_workers; i++) {
        threads_.emplace_back([&, i]() {
            WorkerThread(i);
        });
    }
    Invoke([](unsigned) {});
    Wait();
}

PinnedAffinityExecutor::~PinnedAffinityExecutor() {
    shutdown_.test_and_set();
    Invoke([](unsigned) {});
    for (auto& t : threads_)
        t.join();
}

void PinnedAffinityExecutor::Wait() {
    while (1) {
        const auto old = signal_.load(std::memory_order_relaxed);
        if (old == threads_.size()) {
            signal_.store(0, std::memory_order_relaxed);
            break;
        }
        signal_.wait(old);
    }
}

void PinnedAffinityExecutor::Invoke(std::function<void(unsigned)>&& f) {
    f_ = std::move(f);
    for (unsigned i = 0; i < threads_.size() / atomic_unsigned_type_bits; ++i) {
        bitmap_[i].store(std::numeric_limits<atomic_unsigned_type::value_type>::max(), std::memory_order_release);
        bitmap_[i].notify_all();
    }
    if (threads_.size() % atomic_unsigned_type_bits != 0) {
        bitmap_[bitmap_.size() - 1].store((static_cast<atomic_unsigned_type::value_type>(1) << (threads_.size() % atomic_unsigned_type_bits)) - 1, std::memory_order_release);
        bitmap_[bitmap_.size() - 1].notify_all();
    }
}
