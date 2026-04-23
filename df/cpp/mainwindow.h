#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;


private slots:
    void on_start1_clicked();

    void on_startButton_clicked();

    void on_reportButton_clicked();

    void on_recordbtn_clicked();

    void on_riskbtn_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
