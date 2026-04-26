#include "dialog4.h"
#include "ui_dialog4.h"
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QKeyEvent>
#include <QRegularExpression>
#include <opencv2/opencv.hpp> // Required for VideoCapture

Dialog4::Dialog4(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog4) {
    ui->setupUi(this);
    ui->textEdit->installEventFilter(this);
    ui->textEdit_2->installEventFilter(this);

    timer = new QTimer(this);
    // Connect to the new OpenCV-based capture logic
    connect(timer, &QTimer::timeout, this, &Dialog4::processConfigCapture);
}

Dialog4::~Dialog4() {
    if (m_cap.isOpened()) {
        m_cap.release();
    }
    delete ui;
}

QString Dialog4::readConfigPath() {
    QFile configFile("/app/camera.conf");
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&configFile);
        QString line = in.readLine().trimmed();
        configFile.close();
        return line;
    }
    return QString();
}

void Dialog4::on_pushButton_2_clicked() {
    ui->pushButton_2->setEnabled(false);
    ui->pushButton_2->setText("Connecting...");
    // 2-second delay to allow user to pose
    timer->start(2000); 
}

// 📸 UPDATED: Using OpenCV VideoCapture instead of FFmpeg QProcess
void Dialog4::processConfigCapture() {
    timer->stop();
    QString url = readConfigPath();

    if (url.isEmpty()) {
        QMessageBox::critical(this, "Error", "Config /app/camera.conf is empty.");
        ui->pushButton_2->setEnabled(true);
        ui->pushButton_2->setText("📷 CAPTURE PHOTO");
        return;
    }

    // 1. Open the IP stream
    if (!m_cap.open(url.toStdString())) {
        QMessageBox::warning(this, "Connection Error", "Cannot reach IP Webcam: " + url);
        ui->pushButton_2->setEnabled(true);
        ui->pushButton_2->setText("📷 CAPTURE PHOTO");
        return;
    }

    // 2. Flush buffer (Read 5 frames to get the most recent one)
    cv::Mat frame;
    for(int i = 0; i < 5; i++) {
        m_cap.read(frame);
    }

    if (!frame.empty()) {
        // 3. Convert cv::Mat to QImage for the UI
        // OpenCV uses BGR, Qt uses RGB
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
        
        QImage img((const uchar*) rgbFrame.data, rgbFrame.cols, rgbFrame.rows, rgbFrame.step, QImage::Format_RGB888);
        
        // Use copy() because rgbFrame will be destroyed when this function ends
        processAndDisplayImage(img.copy());
        
        qDebug() << "Success: Image captured from IP Webcam.";
    } else {
        QMessageBox::critical(this, "Capture Error", "Received empty frame from camera.");
    }

    // 4. Cleanup
    m_cap.release(); 
    ui->pushButton_2->setEnabled(true);
    ui->pushButton_2->setText("📷 CAPTURE PHOTO");
}

void Dialog4::processAndDisplayImage(const QImage &img) {
    // 1. Convert QImage to cv::Mat
    // Ensure the format is RGB888 for OpenCV compatibility
    QImage swapped = img.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(swapped.height(), swapped.width(), CV_8UC3, (void*)swapped.bits(), swapped.bytesPerLine());

    // 2. Flip 90 degrees anticlockwise
    cv::Mat rotatedMat;
    cv::rotate(mat, rotatedMat, cv::ROTATE_90_COUNTERCLOCKWISE);

    // 3. Convert back to QImage
    // Important: Use the rotatedMat dimensions (width and height swap)
    QImage result(
        (const uchar*)rotatedMat.data, 
        rotatedMat.cols, 
        rotatedMat.rows, 
        rotatedMat.step, 
        QImage::Format_RGB888
    );

    // 4. Update the global capturedImage (use .copy() to keep data safe)
    capturedImage = result.copy();

    // 5. Display in the UI label
    ui->lbl_captured_image->setPixmap(QPixmap::fromImage(capturedImage).scaled(
        ui->lbl_captured_image->size(), 
        Qt::KeepAspectRatio, 
        Qt::SmoothTransformation
    ));

    qDebug();
}
void Dialog4::on_btnSave_clicked() {
    QString name = ui->textEdit->toPlainText().trimmed();
    QString roll = ui->textEdit_2->toPlainText().trimmed();

    QRegularExpression re("^[a-zA-Z0-9 ]*$"); // Added space support for names
    if (!re.match(roll).hasMatch() || capturedImage.isNull() || name.isEmpty() || roll.isEmpty()) {
        QMessageBox::warning(this, "Error", "Invalid inputs or no image captured.");
        return;
    }

    // Duplicate Check
    QDir baseDir("/app/dataset/");
    for (const QString &dirName : baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFile file(baseDir.absoluteFilePath(dirName + "/info.txt"));
        if (file.open(QIODevice::ReadOnly)) {
            if (QTextStream(&file).readAll().contains("Roll: " + roll)) {
                QMessageBox::critical(this, "Duplicate", "Roll Number already registered!");
                return;
            }
        }
    }

    QString path = getNextFolderPath();
    // Save to the new student folder
    if (capturedImage.save(path + "/image.jpg", "JPG", 95)) {
        QFile info(path + "/info.txt");
        if (info.open(QIODevice::WriteOnly)) {
            QTextStream(&info) << "Name: " << name << "\nRoll: " << roll 
                               << "\nSaved At: " << QDateTime::currentDateTime().toString();
            info.close();
        }

        // --- TRAINING ---
        QProcess *trainProcess = new QProcess(this);
        trainProcess->setWorkingDirectory("/app/src");
        
        ui->btnSave->setEnabled(false);
        ui->btnSave->setText("Training...");

        connect(trainProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
                [=](int exitCode, QProcess::ExitStatus exitStatus){
            
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                QMessageBox::information(this, "Success", "Student Registered and Model Updated!");
            } else {
                QMessageBox::warning(this, "Failed", "Training script failed.");
            }

            ui->btnSave->setEnabled(true);
            ui->btnSave->setText("💾 SAVE TO DATASET");
            ui->textEdit->clear(); 
            ui->textEdit_2->clear();
            ui->lbl_captured_image->clear();
            ui->lbl_captured_image->setText("CAMERA PREVIEW");
            trainProcess->deleteLater();
        });

        trainProcess->start("./train", QStringList()); 
    }
}

QString Dialog4::getNextFolderPath() {
    QString basePath = "/app/dataset/";
    QDir dir(basePath);
    if (!dir.exists()) dir.mkpath(".");
    int index = 0;
    while (dir.exists(QString::number(index))) { index++; }
    dir.mkdir(QString::number(index));
    return basePath + QString::number(index);
}

bool Dialog4::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Space) return true;
    }
    return QDialog::eventFilter(obj, event);
}