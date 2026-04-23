#ifndef DIALOG5_H
#define DIALOG5_H

#include <QDialog>
#include <QImage>
#include <QPixmap>
#include <QString> 
#include <opencv2/opencv.hpp>

namespace Ui { class Dialog5; }

class Dialog5 : public QDialog {
    Q_OBJECT

public:
    explicit Dialog5(QWidget *parent = nullptr);
    ~Dialog5();

private slots:
    void on_btnConfirmDelete_clicked();
    void on_btnCapturePhoto_clicked();

private:
    Ui::Dialog5 *ui;
    
    // --- Helper Functions ---
    // Make sure these names match exactly what you use in dialog5.cpp
    void fetchStudentInfo(QString id); 
    void cleanLogs(QString id);

    // Utility to convert OpenCV frames to Qt Images
    QImage matToQImage(const cv::Mat &mat);
};

#endif // DIALOG5_H