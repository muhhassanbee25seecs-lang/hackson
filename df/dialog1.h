#ifndef DIALOG1_H
#define DIALOG1_H

#include <QDialog>
#include <QTimer>            
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
    void on_pushButton_clicked();   // Toggle Attendance Status
    void on_pushButton_2_clicked(); // Start/Stop IP Camera Stream
    void on_btn_save_clicked();     // Save Data to /app/df/inventory

private:
    Ui::Dialog1 *ui;

    // --- IP CAMERA & STREAMING ---
    // m_cap handles the URL from camera.conf
    cv::VideoCapture m_cap;
    
    // m_timer refreshes the GUI label with the IP stream
    QTimer *m_timer;         

    // --- RECOGNITION LOGIC ---
    void runRecognition();                    // Executes /app/src/recognize
    void readStudentInfo(QString folderName); // Reads from /app/df/dataset/
};

#endif // DIALOG1_H