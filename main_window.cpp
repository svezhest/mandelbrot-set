#include "main_window.h"
#include "ui_main_window.h"
#include <QPainter>
#include <QResizeEvent>
#include <QImage>
#include <algorithm>

main_window::main_window(QWidget *parent)
    : QMainWindow(parent),
      cached_mod(false),
      ui(new Ui::main_window),
      scale(0.005), center_offset(0.),
      parts_number(std::max((std::thread::hardware_concurrency() - 1), 1U)),
      time(clock())
{
    ui->setupUi(this);
    for (size_t i = 0; i < parts_number; ++i) {
        workers.emplace_back(new painting_worker());
        connect(workers[i].get(), &painting_worker::output_changed, this, &main_window::update_output);
    }
    input_changed();
}

main_window::~main_window() {}

void main_window::resizeEvent(QResizeEvent *) {
    input_changed();
}

void main_window::wheelEvent(QWheelEvent *ev) {
    if( ev->delta() > 0 ) {
        scale /= 1.5;
    } else {
        scale *= 1.5;
    }

    input_changed();
}

void main_window::mousePressEvent(QMouseEvent *ev) {
    x = ev->pos().x();
    y = ev->pos().y();
    dx = 0;
    dy = 0;
    cached_mod = true;
    cache_current();
}

void main_window::mouseMoveEvent(QMouseEvent *ev) {
    dx = ev->pos().x() - x;
    dy = ev->pos().y() - y;
    update();
}

void main_window::mouseReleaseEvent(QMouseEvent *ev) {
    time = clock();
    dx = ev->pos().x() - x;
    dy = ev->pos().y() - y;
    cached_mod = false;
    cache.clear();
    if (dx != 0 || dy != 0) {
        center_offset += std::complex<double>(- dx * scale, - dy * scale);
        input_changed();
    }
}

void main_window::cache_current() {
    for_each(workers.begin(), workers.end(), [&](std::unique_ptr<painting_worker>& worker) {
        auto copy = worker->get_output();
        cache.insert(cache.end(), copy.begin(), copy.end());
    });
}

void main_window::paintEvent(QPaintEvent *ev) {
    QMainWindow::paintEvent(ev);    
    int w = width();
    int h = height();

    QImage img(w, h, QImage::Format_RGB888);
    // timeout
    if (static_cast<float>(clock() - time) / CLOCKS_PER_SEC >= AFTERMOVE_BLOCK_TIME) {
        unsigned char *data = img.bits();
        int stride = img.bytesPerLine();

        if (cached_mod) {
            for (int y = 0; y < h; ++y) {
                unsigned char *p = data + y * stride;
                for (int x = 0; x < w; ++x) {
                    size_t t  = (y - dy) * w + (x - dx);
                    *p++ = t >= cache.size() || (x - dx) < 0 || (x - dx) > w ? 0 : cache[t];
                    *p++ = 50;
                    *p++ = 0;
                }
            }
        } else {
            size_t worker_ind = 0;
            auto copy = workers[worker_ind]->get_output();
            auto it = copy.begin();
            for (int y = 0; y < h; ++y) {
                unsigned char *p = data + y * stride;
                for (int x = 0; x < w; ++x) {
                    if (it == copy.end()) {
                        ++worker_ind;
                        if (worker_ind == workers.size()) {
                            break;
                        }
                        copy = workers[worker_ind]->get_output();
                        it = copy.begin();
                    }
                    *p++ = *it++;
                    *p++ = 50;
                    *p++ = 0;
                }
                if (worker_ind == workers.size()) {
                    break;
                }
            }
        }
    }

    QPainter p(this);
    p.drawImage(0, 0, img);
}

void main_window::input_changed() {
    int w = width();
    int h = height();
    int pixels = h * w;
    int part_size = (pixels + parts_number - 1) /
            parts_number;
    for (size_t i = 0; i < parts_number; ++i) {
        workers[i]->set_input({h, w, center_offset, scale},
                                          {i * part_size, (i == parts_number
                                           - 1)? pixels - i * part_size : part_size});
    }
}

void main_window::update_output() {
    this->update();
}
