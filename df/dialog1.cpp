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

// --- UPDATED CONSTRUCTOR ---
// Removed m_camera and m_imageCapture because they are no longer in dialog1.h
Dialog1::Dialog1(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog1)
{
    ui->setupUi(this);
}

// --- UPDATED DESTRUCTOR ---
Dialog1::~Dialog1()
{
    if (m_cap.isOpened()) {
        m_cap.release();
    }
    delete ui;
}

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

    QString dirPath = "/app/inventory";
    QDir dir;
    if (!dir.exists(dirPath)) {
        dir.mkpath(dirPath);
    }

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

        QMessageBox::information(this, "Success", "Attendance data saved for ID: " + id);

        ui->lineEdit->clear();
        ui->lineEdit->setProperty("student_id", "");
        ui->lineEdit_roll->clear();
        ui->lineEdit_2->setText("ABSENT");
        ui->lineEdit_2->setStyleSheet("color: red; background-color: #252a41; border: 1px solid #3a3a4f; border-radius: 5px; padding-left: 15px; font-size: 16px; font-weight: bold;");
    } 
    else {
        QMessageBox::critical(this, "File Error", "Could not write to inventory file.");
    }
}

void Dialog1::on_pushButton_clicked()
{
    QString currentName = ui->lineEdit->text();
    if (currentName.isEmpty() || currentName.contains("Error")) {
        return; 
    }

    QString currentStatus = ui->lineEdit_2->text();

    if (currentStatus == "ABSENT") {
        ui->lineEdit_2->setText("PRESENT");
        ui->lineEdit_2->setStyleSheet("color: #2ecc71; background-color: #252a41; border: 1px solid #3a3a4f; border-radius: 5px; padding-left: 15px; font-size: 16px; font-weight: bold;");
    } 
    else {
        ui->lineEdit_2->setText("ABSENT");
        ui->lineEdit_2->setStyleSheet("color: red; background-color: #252a41; border: 1px solid #3a3a4f; border-radius: 5px; padding-left: 15px; font-size: 16px; font-weight: bold;");
    }
}

void Dialog1::on_pushButton_2_clicked()
{
    printf("Starting IP camera...\n");

    QFile file("camera.conf");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        printf("camera.conf missing\n");
        return;
    }

    QString url = QTextStream(&file).readLine().trimmed();
    file.close();

    if (url.isEmpty()) {
        printf("No camera URL\n");
        return;
    }

    m_cap.open(url.toStdString());

    if (!m_cap.isOpened()) {
        printf("Camera failed to open\n");
        return;
    }

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        cv::Mat frame;
        m_cap >> frame;
        if (frame.empty()) return;

        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        QImage img(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
        ui->label->setPixmap(QPixmap::fromImage(img).scaled(ui->label->size(), Qt::KeepAspectRatio));
    });
    timer->start(30);
}

// --- UPDATED PROCESS CAPTURED IMAGE ---
void Dialog1::processCapturedImage(int id, const QImage &preview)
{
    Q_UNUSED(id);
    QString fileName = "/app/test.jpg";
    
    if(preview.save(fileName, "JPG")) {
        printf("SUCCESS: Image saved. Starting Recognition...\n");
        runRecognition();
    } else {
        printf("ERROR: Could not save image.\n");
    }

    // Removed reference to m_camera
    ui->pushButton_2->setEnabled(true);
    ui->pushButton_2->setText("START CAMERA");
}

void Dialog1::runRecognition()
{
    QProcess *recognizeProcess = new QProcess(this);
    recognizeProcess->setWorkingDirectory("/app/src");

    connect(recognizeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, recognizeProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            QString fullOutput = recognizeProcess->readAllStandardOutput();
            QStringList lines = fullOutput.split("\n");
            QString studentId = "";

            for (const QString &line : lines) {
                if (line.contains("Recognized Student ID:")) {
                    studentId = line.section(':', 1).section('|', 0).trimmed();
                    break; 
                }
            }

            if (!studentId.isEmpty()) {
                readStudentInfo(studentId);
            } 
            else {
                QMessageBox::warning(this, "Recognition Failed", "Student not recognized.");
                ui->lineEdit->setText("Unknown");
                ui->lineEdit->setProperty("student_id", "");
                ui->lineEdit_roll->setText("N/A");
            }
        }
        recognizeProcess->deleteLater();
    });

    // Separation of arguments is cleaner for the compiler
    recognizeProcess->start("./recognize", QStringList());
}

void Dialog1::readStudentInfo(QString folderName)
{
    QString infoPath = "/app/dataset/" + folderName + "/info.txt";
    QFile file(infoPath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        ui->lineEdit->clear();
        ui->lineEdit_roll->clear();
        ui->lineEdit->setProperty("student_id", folderName);

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("Name:", Qt::CaseInsensitive)) {
                ui->lineEdit->setText(line.section(':', 1).trimmed());
            }
            if (line.startsWith("Roll:", Qt::CaseInsensitive)) {
                ui->lineEdit_roll->setText(line.section(':', 1).trimmed());
            }
        }
        file.close();
        ui->lineEdit_2->setText("ABSENT");
        ui->lineEdit_2->setStyleSheet("color: red; background-color: #252a41; border: 1px solid #3a3a4f; border-radius: 5px; padding-left: 15px; font-size: 16px; font-weight: bold;");
    } 
    else {
        ui->lineEdit->setText("Error: Data Missing");
        ui->lineEdit->setProperty("student_id", "N/A");
        ui->lineEdit_roll->setText("N/A");
    }
}