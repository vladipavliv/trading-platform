/**
 * @author Vladimir Pavliv
 * @date 2025-07-26
 */

#ifndef HFT_MONITOR_MAINWINDOW_HPP
#define HFT_MONITOR_MAINWINDOW_HPP

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace hft::monitor {

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  Ui::MainWindow *ui;

  void setupMonitoring();
};

} // namespace hft::monitor

#endif // HFT_MONITOR_MAINWINDOW_HPP