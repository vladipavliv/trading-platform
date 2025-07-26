/**
 * @author Vladimir Pavliv
 * @date 2025-04-11
 */

#include <iostream>

#include <QApplication>

#include "config/monitor_config.hpp"
#include "logging.hpp"
#include "monitor_control_center.hpp"
#include "ui/main_window.h"

int main(int argc, char *argv[]) {
  using namespace hft;
  using namespace monitor;
  try {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();

    MonitorConfig::load("monitor_config.ini");
    LOG_INIT(MonitorConfig::cfg.logOutput);

    MonitorControlCenter monitorCc;
    monitorCc.start();
  } catch (const std::exception &e) {
    std::cerr << "Exception caught in main {}" << e.what() << std::endl;
  }
  return 0;
}
