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

Dialog4::Dialog4(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog4) {
    ui->setupUi(this);
    ui->textEdit->installEventFilter(this);
    ui->textEdit_2->installEventFilter(this);

    // Initialize Camera and Timer
    camera = new QCamera(this);
    imageCapture = new QCameraImageCapture(camera, this);
    timer = new QTimer(this);

    // Connect signals
    connect(timer, &QTimer::timeout, this, &Dialog4::captureImage);
    connect(imageCapture, &QCameraImageCapture::imageCaptured, this, &Dialog4::imageCaptured);
}

bool Dialog4::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Space) {
            return true; // Filter out (ignore) the space key
        }
    }
    return QDialog::eventFilter(obj, event);
}

Dialog4::~Dialog4() {
    delete ui;
}

// 📸 Step 1: Open Camera and start 2s countdown
void Dialog4::on_pushButton_2_clicked() {
    qDebug() << "Starting camera for 2-second warm-up...";
    camera->start(); 
    
    ui->pushButton_2->setEnabled(false);
    ui->pushButton_2->setText("Capturing in 2s...");
    
    timer->start(2000); 
}

// 📸 Step 2: Perform capture and turn off camera
void Dialog4::captureImage() {
    timer->stop();
    if (imageCapture->isReadyForCapture()) {
        imageCapture->capture();
    } else {
        qDebug() << "Camera not ready for capture.";
    }
    
    camera->stop(); // Power off camera to save resources
    ui->pushButton_2->setEnabled(true);
    ui->pushButton_2->setText("📷 CAPTURE PHOTO");
}

// 🖼 Step 3: Show result in the UI (Fixed unused parameter warning)
void Dialog4::imageCaptured(int, const QImage &preview) {
    capturedImage = preview;
    ui->lbl_captured_image->setPixmap(QPixmap::fromImage(preview).scaled(
        ui->lbl_captured_image->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    qDebug() << "Image displayed in preview.";
}

// 📁 Helper: Create /app/dataset/0, 1, 2...
QString Dialog4::getNextFolderPath() {
    QString basePath = "/app/dataset/";
    QDir dir(basePath);
    if (!dir.exists()) dir.mkpath(".");

    int index = 0;
    while (dir.exists(QString::number(index))) { 
        index++; 
    }
    
    QString newPath = basePath + QString::number(index);
    dir.mkdir(QString::number(index));
    return newPath;
}

void Dialog4::on_btnSave_clicked() {
    // 1. Get Clean Data
    QString name = ui->textEdit->toPlainText().trimmed();
    QString roll = ui->textEdit_2->toPlainText().trimmed();

    if (capturedImage.isNull()) {
        QMessageBox::warning(this, "Error", "Capture a photo first!");
        return;
    }
    if (name.isEmpty() || roll.isEmpty()) {
        QMessageBox::warning(this, "Error", "Fill in all fields!");
        return;
    }

    // 2. DUPLICATE CHECK (Check if Roll Number already exists)
    QDir baseDir("/app/dataset/");
    QStringList subDirs = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString &dirName : subDirs) {
        QFile checkFile(baseDir.absoluteFilePath(dirName + "/info.txt"));
        if (checkFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&checkFile);
            QString content = in.readAll();
            checkFile.close();
            
            if (content.contains("Roll: " + roll)) {
                QMessageBox::critical(this, "Error", "Already exist student (Roll Number matched)!");
                return; 
            }
        }
    }

    // 3. SAVE DATA + TIMESTAMP
    QString path = getNextFolderPath();
    if (!capturedImage.save(path + "/image.jpg")) {
        QMessageBox::critical(this, "Error", "Save failed. Check permissions.");
        return;
    }

    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    QFile file(path + "/info.txt");
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream out(&file);
        out << "Name: " << name << "\n";
        out << "Roll: " << roll << "\n";
        out << "Saved At: " << currentDateTime; 
        file.close();
    }

    // 4. RUN TRAINING
    QProcess *trainProcess = new QProcess(this);
    trainProcess->setWorkingDirectory("/app/src");
    ui->btnSave->setEnabled(false);
    ui->btnSave->setText("Training...");

    connect(trainProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            QMessageBox::information(this, "Success", "Student saved and Model updated!");
        }
        ui->btnSave->setEnabled(true);
        ui->btnSave->setText("💾  SAVE TO DATASET");
        trainProcess->deleteLater();
    });

    trainProcess->start("./train");

    

// Reset UI
    ui->textEdit->clear();
    ui->textEdit_2->clear();
    ui->lbl_captured_image->clear();
    ui->lbl_captured_image->setText("CAMERA PREVIEW");
}
