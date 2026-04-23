#ifndef DIALOG4_H
#define DIALOG4_H

#include <QDialog>
#include <QTimer>
#include <QCamera>
#include <QCameraImageCapture>
#include <QImage>
#include <QFile>
#include <QTextStream>

namespace Ui {
class Dialog4;
}

class Dialog4 : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog4(QWidget *parent = nullptr);
    ~Dialog4();

private slots:
    void on_pushButton_2_clicked(); // Capture button
    void on_btnSave_clicked();      // Save button
    void captureImage();            // Timer timeout
    void imageCaptured(int id, const QImage &preview);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
private:
    Ui::Dialog4 *ui;
    QCamera *camera;
    QCameraImageCapture *imageCapture;
    QTimer *timer;
    QImage capturedImage;

    QString getNextFolderPath();
};

#endif // DIALOG4_H