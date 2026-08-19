#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QCloseEvent>
#include <QTime>
#include "camerathread.h"
#include "cardbmanager.h"

#include <QDir>
#include <QChar>

class ProcessingDialog;//dialog 생성하고 추가하기

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void handle_data(const QImage &image);

    void on_btnTakeAPicture_clicked();
    void handle_ai_result(const QString& carnum); // ai 결과판 결과 수신

private:
    Ui::MainWindow *ui;
    CameraThread *camera_thread;
    QImage current_image;
    CarDBManager carDB; //db 객체 생성

    ProcessingDialog * processingDialog;
};
#endif // MAINWINDOW_H