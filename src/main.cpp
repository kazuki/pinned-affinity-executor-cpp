#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <pinned_affinity_executor.hpp>
#include "BS_thread_pool/BS_thread_pool.hpp"


double avg90percentile(std::vector<double>& times) {
    std::sort(times.begin(), times.end());
    double sum = 0.0;
    const auto s = times.size() / 20;
    const auto e = times.size() * 95 / 100;
    for (auto i = s; i < e; i++) {
        sum += times[i];
    }
    times.clear();
    return sum / (e - s);
}

int main() {
    using namespace std::chrono;

    PinnedAffinityExecutor executor;
    BS::thread_pool bs_threadpool;

    const int N_LOOP = 10000;
    std::vector<double> times;
    times.reserve(N_LOOP);

    for (int i = 0; i < N_LOOP; ++i) {
        const auto begin = high_resolution_clock::now();
        executor.Invoke([](unsigned) {});
        executor.Wait();
        const auto end = high_resolution_clock::now();
        times.push_back(duration_cast<duration<double, std::milli>>(end - begin).count());
    }
    std::cout << "latency: " << avg90percentile(times) << "ms" << std::endl;

    for (int i = 0; i < N_LOOP; ++i) {
        const auto begin = high_resolution_clock::now();
        for (unsigned j = 0; j < std::thread::hardware_concurrency(); ++j) {
            bs_threadpool.push_task([]() {});
        }
        bs_threadpool.wait_for_tasks();
        const auto end = high_resolution_clock::now();
        times.push_back(duration_cast<duration<double, std::milli>>(end - begin).count());
    }
    std::cout << "BS::thread_pool latency: " << avg90percentile(times) << "ms" << std::endl;
}
