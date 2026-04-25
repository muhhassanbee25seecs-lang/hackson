#ifndef DIALOG1_H
#define DIALOG1_H

#include <QDialog>
#include <QImage>
#include <opencv2/opencv.hpp>

namespace Ui {
class Dialog1;
}

class Dialog1 : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog1(QWidget *parent = nullptr);
    ~Dialog1();

private slots:
    void on_pushButton_clicked();   // Toggle Attendance Button
    void on_pushButton_2_clicked(); // Start Camera Button
    void on_btn_save_clicked();     // Save Data Button
    void processCapturedImage(int id, const QImage &preview);

private:
    Ui::Dialog1 *ui;

    // NEW CAMERA SYSTEM
    cv::VideoCapture m_cap;

    QString getCameraUrl();

    void runRecognition();
    void readStudentInfo(QString folderName);
};

#endif // DIALOG1_H