#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>
#include <QObject>
#include <complex>

struct global {
    int h;
    int w;
    std::complex<double> center_offset;
    double scale;
};

struct particular {
    size_t start;
    size_t pixel_count;
};

class painting_worker : public QObject
{
    Q_OBJECT

public:
    painting_worker();
    ~painting_worker();

    void set_input (global, particular);
    std::vector<unsigned char> get_output() const;

signals:
    void output_changed();

private:
    void thread_proc();
    void calculate(uint64_t last_input_version,
                   global &window_info, particular &thread_info);
    void store_result(std::vector<unsigned char> &result);

private slots:
    void notify_output();

private:
    global window_info;
    particular thread_info;
    mutable std::mutex m;
    std::atomic<uint64_t> input_version;
    std::condition_variable input_changed;
    std::vector<unsigned char> output;
    bool notify_output_queued = false;
    std::thread worker_thread;
    std::vector<std::complex<double>> z;
    std::vector<std::complex<double>> c;
    std::vector<unsigned char> ans;

    static uint64_t const INPUT_VERSION_QUIT = 0;
};
