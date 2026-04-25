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
    m_camera(nullptr),           
    m_imageCapture(nullptr)
{
    ui->setupUi(this);
}

Dialog1::~Dialog1()
{
    if (m_camera) {
        m_camera->stop();
    }
    delete ui;
}

// Add this to dialog1.cpp
#include <QMessageBox>
void Dialog1::on_btn_save_clicked()
{
    // 1. Collect data from the UI
    QString name = ui->lineEdit->text();
    QString roll = ui->lineEdit_roll->text();
    QString status = ui->lineEdit_2->text();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // --- FIX: Retrieve the ID stored during recognition ---
    // This fetches the folder name (0, 1, 2...) that was saved in readStudentInfo
    QString id = ui->lineEdit->property("student_id").toString();

    // 2. Validation: Don't save if fields are empty
    if (name.isEmpty() || roll.isEmpty() || name.contains("Error")) {
        QMessageBox::warning(this, "Save Error", "No valid student data to save!");
        return;
    }

    // 3. Ensure the inventory directory exists
    QString dirPath = "/app/inventory";
    QDir dir;
    if (!dir.exists(dirPath)) {
        dir.mkpath(dirPath);
    }

    // 4. Open the file in Append mode
    QFile file(dirPath + "/attendance_records.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << "------------------------------\n";
        out << "Timestamp: " << timestamp << "\n";
        out << "Name:      " << name << "\n";
        out << "Roll:      " << roll << "\n";
        out << "ID:        " << id << "\n"; // Now 'id' is declared and has a value
        out << "Status:    " << status << "\n";
        out << "------------------------------\n";
        file.close();

        // 5. Feedback
        QMessageBox::information(this, "Success", "Attendance data saved for ID: " + id);

        // 6. Reset UI for next student
        ui->lineEdit->clear();
        ui->lineEdit->setProperty("student_id", ""); // Clear the stored ID
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
    // 1. Check if a name was actually detected first
    QString currentName = ui->lineEdit->text();
    if (currentName.isEmpty() || currentName.contains("Error")) {
        return; 
    }

    // 2. Get the current status text
    QString currentStatus = ui->lineEdit_2->text();

    // 3. Toggle Logic
    if (currentStatus == "ABSENT") {
        // Switch to PRESENT
        ui->lineEdit_2->setText("PRESENT");
        ui->lineEdit_2->setStyleSheet("color: #2ecc71; background-color: #252a41; border: 1px solid #3a3a4f; border-radius: 5px; padding-left: 15px; font-size: 16px; font-weight: bold;");
        printf("Attendance toggled: %s is now PRESENT\n", currentName.toStdString().c_str());
    } 
    else {
        // Switch back to ABSENT
        ui->lineEdit_2->setText("ABSENT");
        ui->lineEdit_2->setStyleSheet("color: red; background-color: #252a41; border: 1px solid #3a3a4f; border-radius: 5px; padding-left: 15px; font-size: 16px; font-weight: bold;");
        printf("Attendance toggled: %s is now ABSENT\n", currentName.toStdString().c_str());
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

        QImage img(frame.data,
                   frame.cols,
                   frame.rows,
                   frame.step,
                   QImage::Format_RGB888);

        ui->label->setPixmap(QPixmap::fromImage(img));
    });

    timer->start(30);
}
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

    if (m_camera) {
        m_camera->stop();
    }
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
                    // Extract numeric ID from output
                    studentId = line.section(':', 1).section('|', 0).trimmed();
                    break; 
                }
            }

            if (!studentId.isEmpty()) {
                readStudentInfo(studentId); // Pass the ID (e.g., "0", "1")
            } 
            else {
                QMessageBox::warning(this, "Recognition Failed", "Student not recognized.");
                ui->lineEdit->setText("Unknown");
                ui->lineEdit->setProperty("student_id", ""); // Clear ID
                ui->lineEdit_roll->setText("N/A");
            }
        }
        recognizeProcess->deleteLater();
    });

    recognizeProcess->start("./recognize", QStringList());
}

void Dialog1::readStudentInfo(QString folderName)
{
    // folderName is our ID (0, 1, 2...)
    QString infoPath = "/app/dataset/" + folderName + "/info.txt";
    QFile file(infoPath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        
        ui->lineEdit->clear();
        ui->lineEdit_roll->clear();
        
        // Store the folderName (ID) inside the lineEdit property so btn_save can find it
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