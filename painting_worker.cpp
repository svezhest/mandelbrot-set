#include "painting_worker.h"

painting_worker::painting_worker()
    : input_version(INPUT_VERSION_QUIT),
      worker_thread([this] { thread_proc(); })
{}

painting_worker::~painting_worker()
{
    input_version = INPUT_VERSION_QUIT;
    input_changed.notify_all();
    worker_thread.join();
}

void painting_worker::set_input(global new_window_info, particular new_thread_info)
{
    {
        std::lock_guard lg(m);
        thread_info = new_thread_info;
        window_info = new_window_info;
        ++input_version;
    }
    input_changed.notify_all();
}

void painting_worker::thread_proc()
{
    uint64_t last_input_version = 0;
    for (;;)
    {
        global window_info_copy;
        particular thread_info_copy;
        {
            std::unique_lock lg(m);
            input_changed.wait(lg, [&]
            {
                return input_version != last_input_version;
            });

            last_input_version = input_version;
            if (last_input_version == INPUT_VERSION_QUIT)
                break;
            window_info_copy = window_info;
            thread_info_copy = thread_info;
            ans.resize(thread_info.pixel_count);
            z.resize(thread_info.pixel_count);
            c.resize(thread_info.pixel_count);
            output.resize(thread_info.pixel_count);
            fill(ans.begin(), ans.end(), 0);
            fill(output.begin(), output.end(), 0);
            fill(z.begin(), z.end(), 0);
        }
        calculate(last_input_version, window_info_copy, thread_info_copy);
    }
}

void painting_worker::calculate(uint64_t last_input_version,
                                global &window_info, particular &thread_info) {
    size_t const MAX_STEPS = 200;
    for (size_t step = 0; step < MAX_STEPS; ++step) {
        size_t ind = 0;
        for (int y = thread_info.start / window_info.w; y < window_info.h && ind < thread_info.pixel_count; ++y) {
            int x = 0;
            if (ind == 0) {
                x = thread_info.start % window_info.w;
            }
            for (; x < window_info.w && ind < thread_info.pixel_count; ++x, ++ind) {
                if (last_input_version != input_version) {
                    store_result(ans);
                    return;
                }
                if (step == 0) {
                    c[ind] = std::complex<double>(x - window_info.w / 2, y - window_info.h / 2);
                    c[ind] *= window_info.scale;
                    c[ind] += window_info.center_offset;
                }
                if (ans[ind] != 0) {
                    continue;
                }
                if (std::abs(z[ind]) >= 2.) {
                    ans[ind] = ((step % 51) / 50.) * 255;
                }
                z[ind] = z[ind] * z[ind] + c[ind];
            }
        }
        store_result(ans);
    }
}

std::vector<unsigned char> painting_worker::get_output() const
{
    std::lock_guard lg(m);
    return output;
}

void painting_worker::store_result(std::vector<unsigned char> &result)
{
    std::lock_guard lg(m);
    output.swap(result);

    if (!notify_output_queued)
    {
        QMetaObject::invokeMethod(this, "notify_output");
        notify_output_queued = true;
    }
}

void painting_worker::notify_output()
{
    {
        std::lock_guard lg(m);
        notify_output_queued = false;
    }
    emit output_changed();
}
