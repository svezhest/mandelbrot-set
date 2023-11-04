#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <memory>
#include "painting_worker.h"
#include <ctime>

QT_BEGIN_NAMESPACE
namespace Ui { class main_window; }
QT_END_NAMESPACE

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    main_window(QWidget *parent = nullptr);
    ~main_window() override;

    void input_changed();
    void cache_current();
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;


private slots:
    void update_output();

private:
    bool cached_mod;
    std::unique_ptr<Ui::main_window> ui;
    std::vector<std::unique_ptr<painting_worker>> workers;
    std::vector<unsigned char> cache;
    double scale;
    std::complex<double> center_offset;
    int x, y, dx, dy;
    size_t parts_number;
    clock_t time;
    const float AFTERMOVE_BLOCK_TIME = 0.005;
};

#endif // MAIN_WINDOW_H
