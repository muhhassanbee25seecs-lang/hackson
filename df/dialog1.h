#ifndef DIALOG1_H
#define DIALOG1_H

#include <QDialog>
#include <QCamera>
#include <QCameraInfo>
#include <QCameraImageCapture>
#include <QImage>

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
    void on_btn_save_clicked();     // Save Data Button (if you plan to use it)
    void processCapturedImage(int id, const QImage &preview);

private:
    Ui::Dialog1 *ui;
    QCamera *m_camera;
    QCameraImageCapture *m_imageCapture;

    // --- ADD THESE TWO LINES ---
    void runRecognition();
    void readStudentInfo(QString folderName);
};

#endif // DIALOG1_H
