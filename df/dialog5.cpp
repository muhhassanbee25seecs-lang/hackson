#include "dialog5.h"
#include "ui_dialog5.h"
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <opencv2/opencv.hpp>

Dialog5::Dialog5(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog5)
{
    ui->setupUi(this);
}

Dialog5::~Dialog5()
{
    delete ui;
}

// 1. CAPTURE & RECOGNIZE
void Dialog5::on_btnCapturePhoto_clicked()
{
    ui->btnCapturePhoto->setEnabled(false);
    ui->btnCapturePhoto->setText("Recognizing...");

    cv::VideoCapture cap(0, cv::CAP_V4L2); 
    if (!cap.isOpened()) {
        QMessageBox::critical(this, "Camera Error", "Unable to access the camera.");
        ui->btnCapturePhoto->setEnabled(true);
        ui->btnCapturePhoto->setText("📷 CAPTURE PHOTO");
        return;
    }

    cv::Mat frame;
    cap >> frame;
    cap.release();

    if (frame.empty()) {
        ui->btnCapturePhoto->setEnabled(true);
        ui->btnCapturePhoto->setText("📷 CAPTURE PHOTO");
        return;
    }

    // Show image in UI
    cv::Mat rgbFrame;
    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
    QImage qimg((const unsigned char*)(rgbFrame.data), rgbFrame.cols, rgbFrame.rows, rgbFrame.step, QImage::Format_RGB888);
    ui->student_picture_frame->setPixmap(QPixmap::fromImage(qimg.copy()).scaled(
        ui->student_picture_frame->size(), 
        Qt::KeepAspectRatioByExpanding, 
        Qt::SmoothTransformation
    ));

    cv::imwrite("/app/test.jpg", frame);

    QProcess *recognizeProcess = new QProcess(this);
    recognizeProcess->setWorkingDirectory("/app/src"); 

    connect(recognizeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, recognizeProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            QString fullOutput = recognizeProcess->readAllStandardOutput();
            QString studentId = "";

            QStringList lines = fullOutput.split("\n");
            for (const QString &line : lines) {
                if (line.contains("Recognized Student ID:")) {
                    studentId = line.section(':', 1).section('|', 0).trimmed();
                    break;
                }
            }

            if (!studentId.isEmpty() && studentId != "-1") {
                ui->textEdit_id->setProperty("real_id", studentId);
                fetchStudentInfo(studentId);
            } else {
                ui->textEdit_id->setPlainText("Unknown Student");
                ui->textEdit_id->setProperty("real_id", "");
            }
        }
        ui->btnCapturePhoto->setEnabled(true);
        ui->btnCapturePhoto->setText("📷 CAPTURE PHOTO");
        recognizeProcess->deleteLater();
    });

    recognizeProcess->start("./recognize");
}

// 2. FETCH STUDENT NAME
void Dialog5::fetchStudentInfo(QString id) {
    QString infoPath = "/app/dataset/" + id + "/info.txt";
    QFile file(infoPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("Name:", Qt::CaseInsensitive)) {
                ui->textEdit_id->setPlainText(line.section(':', 1).trimmed());
                return;
            }
        }
    }
    ui->textEdit_id->setPlainText("ID: " + id);
}

// 3. CLEAN TEXT LOGS (Removes data from attendance_records.txt)
#include <QRegularExpression> // Add this include at the top of dialog5.cpp

// ... includes stay the same ...

// 3. CLEAN TEXT LOGS (Removes data from attendance_records.txt)
void Dialog5::cleanLogs(QString id)
{
    QString filePath = "/app/inventory/attendance_records.txt";
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return; 
    }

    QStringList filteredContent;
    QStringList currentBlock;
    bool blockContainsId = false;

    // Matches "ID:" followed by spaces and your ID number exactly
    QRegularExpression idPattern("^ID:\\s*" + QRegularExpression::escape(id.trimmed()) + "$");

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        currentBlock.append(line);

        // Check if current line in the block is the target ID
        if (line.trimmed().contains(idPattern)) {
            blockContainsId = true;
        }

        // Detect block separator
        if (line.contains("------------------------------")) {
            if (!blockContainsId) {
                filteredContent.append(currentBlock);
            }
            currentBlock.clear();
            blockContainsId = false;
        }
    }
    
    // Safety for final block if separator is missing at the very end
    if (!currentBlock.isEmpty() && !blockContainsId) {
        filteredContent.append(currentBlock);
    }
    
    file.close();

    // Overwrite with the clean data
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &blockLine : filteredContent) {
            out << blockLine << "\n";
        }
        file.close();
    }
}

// 4. DELETE BUTTON (Logic to handle both Camera and Manual Folder Name)
void Dialog5::on_btnConfirmDelete_clicked()
{
    // Try to get ID from camera recognition property first
    QString studentId = ui->textEdit_id->property("real_id").toString();
    
    // If empty (manual mode), get the text from the field (e.g., "0" or "1")
    if (studentId.isEmpty()) {
        studentId = ui->textEdit_id->toPlainText().trimmed();
    }

    // Final check before proceeding
    if (studentId.isEmpty() || studentId == "Unknown Student") {
        QMessageBox::warning(this, "Error", "Capture a photo or enter a Folder ID (0, 1...) to delete.");
        return;
    }

    auto reply = QMessageBox::question(this, "Confirm Deletion", 
                                     "Delete ALL logs and dataset folder for ID: " + studentId + "?",
                                     QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 1. Delete matching entries from the log file
        cleanLogs(studentId);

        // 2. Delete the physical folder in the dataset
        QString datasetPath = "/app/dataset/" + studentId;
        QDir dir(datasetPath);
        
        bool folderRemoved = false;
        if (dir.exists()) {
            folderRemoved = dir.removeRecursively();
        }

        // 3. Status update
        if (folderRemoved) {
            QMessageBox::information(this, "Success", "Folder and logs for ID " + studentId + " deleted.");
        } else {
            QMessageBox::information(this, "Logs Cleaned", "Attendance logs for ID " + studentId + " were removed, but no folder was found at " + datasetPath);
        }

        // 4. Reset UI
        ui->textEdit_id->clear();
        ui->student_picture_frame->clear();
        ui->textEdit_id->setProperty("real_id", ""); // Clear the property
    }
}