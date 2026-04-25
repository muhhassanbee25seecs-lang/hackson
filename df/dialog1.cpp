#include "dialog1.h"
#include "ui_dialog1.h"
#include <QDateTime>
#include <opencv2/opencv.hpp>
#include <QTimer>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>

Dialog1::Dialog1(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog1),
    m_timer(nullptr) // Initialize the timer pointer
{
    ui->setupUi(this);
}

Dialog1::~Dialog1()
{
    if (m_cap.isOpened()) {
        m_cap.release();
    }
    delete ui;
}

// --- KEEP YOUR EXISTING SAVE AND TOGGLE LOGIC ---
void Dialog1::on_btn_save_clicked()
{
    QString name = ui->lineEdit->text();
    QString roll = ui->lineEdit_roll->text();
    QString status = ui->lineEdit_2->text();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString id = ui->lineEdit->property("student_id").toString();

    if (name.isEmpty() || roll.isEmpty() || name.contains("Error")) {
        QMessageBox::warning(this, "Save Error", "No valid student data to save!");
        return;
    }

    QString dirPath = "/app/df/inventory"; // Path based on your explorer
    QDir dir;
    if (!dir.exists(dirPath)) dir.mkpath(dirPath);

    QFile file(dirPath + "/attendance_records.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << "------------------------------\n";
        out << "Timestamp: " << timestamp << "\n";
        out << "Name:      " << name << "\n";
        out << "Roll:      " << roll << "\n";
        out << "ID:        " << id << "\n";
        out << "Status:    " << status << "\n";
        out << "------------------------------\n";
        file.close();
        QMessageBox::information(this, "Success", "Attendance saved!");
    }
}

void Dialog1::on_pushButton_clicked()
{
    if (ui->lineEdit->text().isEmpty()) return;
    
    if (ui->lineEdit_2->text() == "ABSENT") {
        ui->lineEdit_2->setText("PRESENT");
        ui->lineEdit_2->setStyleSheet("color: #2ecc71; background-color: #252a41; border: 1px solid #3a3a4f; border-radius: 5px; padding-left: 15px; font-size: 16px; font-weight: bold;");
    } else {
        ui->lineEdit_2->setText("ABSENT");
        ui->lineEdit_2->setStyleSheet("color: red; background-color: #252a41; border: 1px solid #3a3a4f; border-radius: 5px; padding-left: 15px; font-size: 16px; font-weight: bold;");
    }
}

// --- UPDATED IP WEBCAM LOGIC ---
void Dialog1::on_pushButton_2_clicked()
{
    // Toggle: If camera is running, stop it
    if (m_cap.isOpened()) {
        if (m_timer) m_timer->stop();
        m_cap.release();
        ui->label->clear();
        ui->pushButton_2->setText("START CAMERA");
        return;
    }

    // 1. Read the IP URL from camera.conf
    QFile file("/app/camera.conf"); 
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "camera.conf missing in /app/");
        return;
    }
    QString url = QTextStream(&file).readLine().trimmed();
    file.close();

    // 2. Open the IP Stream
    m_cap.open(url.toStdString());
    if (!m_cap.isOpened()) {
        QMessageBox::warning(this, "Error", "Cannot reach IP Webcam: " + url);
        return;
    }

    ui->pushButton_2->setText("STOP CAMERA");

    // 3. Start Timer to refresh the frame and auto-capture
    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this]() {
            cv::Mat frame;
            m_cap >> frame;
            if (frame.empty()) return;

            // Display current frame
            cv::Mat rgb;
            cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
            QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
            ui->label->setPixmap(QPixmap::fromImage(img).scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        });
    }

    m_timer->start(30);

    // 4. Trigger Recognition capture after 2 seconds (simulating your original logic)
    QTimer::singleShot(2000, this, [this]() {
        if (m_cap.isOpened()) {
            cv::Mat captureFrame;
            m_cap >> captureFrame;
            if (!captureFrame.empty()) {
                cv::imwrite("/app/test.jpg", captureFrame);
                printf("Capture successful! Running Recognition...\n");
                runRecognition();
            }
        }
    });
}

void Dialog1::runRecognition()
{
    QProcess *recognizeProcess = new QProcess(this);
    recognizeProcess->setWorkingDirectory("/app/src");

    connect(recognizeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, recognizeProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            QString output = recognizeProcess->readAllStandardOutput();
            QString studentId = "";
            QStringList lines = output.split("\n");

            for (const QString &line : lines) {
                if (line.contains("Recognized Student ID:")) {
                    studentId = line.section(':', 1).trimmed();
                    break; 
                }
            }

            if (!studentId.isEmpty()) {
                readStudentInfo(studentId);
            } else {
                QMessageBox::warning(this, "Failed", "Not Recognized.");
            }
        }
        recognizeProcess->deleteLater();
    });

    recognizeProcess->start("./recognize");
}

void Dialog1::readStudentInfo(QString folderName)
{
    QString infoPath = "/app/df/dataset/" + folderName + "/info.txt";
    QFile file(infoPath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        ui->lineEdit->setProperty("student_id", folderName);

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("Name:", Qt::CaseInsensitive)) ui->lineEdit->setText(line.section(':', 1).trimmed());
            if (line.startsWith("Roll:", Qt::CaseInsensitive)) ui->lineEdit_roll->setText(line.section(':', 1).trimmed());
        }
        file.close();
        ui->lineEdit_2->setText("ABSENT");
    }
}