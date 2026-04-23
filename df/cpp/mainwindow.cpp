#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialog1.h"
#include "dialog2.h"
#include "dialog3.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    this->setStyleSheet(R"(
QMainWindow {
    background-color: #1e1e2f;
}


QWidget {
    background-color: #1e1e2f;
    color: white;
}
)");

    ui->startButton->setIcon(QIcon("C:/Users/PC/Downloads/camera.png"));
    ui->reportButton->setIcon(QIcon("C:/Users/PC/Downloads/report.png"));

    ui->startButton->setIconSize(QSize(60, 60));

    ui->reportButton->setIconSize(QSize(60, 60));


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_start1_clicked()
{
    Dialog1 dlg(this);
    dlg.exec();
}


void MainWindow::on_startButton_clicked()
{
    Dialog1 dlg(this);
    dlg.exec();
}

void MainWindow::on_reportButton_clicked()
{
    Dialog2 *dlg = new Dialog2(this);
    dlg->show();
}


void MainWindow::on_recordbtn_clicked()
{
    Dialog2 dlg(this);
    dlg.exec();
}


void MainWindow::on_riskbtn_clicked()
{
    Dialog3 dlg(this);
    dlg.exec();
}

