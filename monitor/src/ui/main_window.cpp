/**
 * @author Vladimir Pavliv
 * @date 2025-07-26
 */

#include <QDateTime>
#include <QTimer>

#include "main_window.h"
#include "ui_main_window.h"

namespace hft::monitor {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setupMonitoring();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupMonitoring() {
  // Example: update a label every second
  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, [this]() {
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    ui->statusLabel->setText("Monitoring... " + time);
  });
  timer->start(1000);
}

} // namespace hft::monitor