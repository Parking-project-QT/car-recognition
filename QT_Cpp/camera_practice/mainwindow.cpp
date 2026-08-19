#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processingdialog.h"

#include "enterdialog.h"
#include "exitdialog.h"

#include <QPixmap>
#include <QDebug>
#include <QDir>

// ============================================================================
// MainWindow 생성자 (초기화 및 시작)
// ============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), processingDialog(nullptr) {
    ui->setupUi(this); // UI 디자이너 파일(*.ui)의 컴포넌트들을 화면에 배치 및 초기화

    // 1. 백그라운드에서 카메라를 구동할 스레드 객체 생성
    camera_thread = new CameraThread(this);
    
    // 2. 스레드(영상 송신)와 메인 UI(영상 수신)를 시그널-슬롯으로 연결
    connect(camera_thread, SIGNAL(send_image(const QImage&)),
            this, SLOT(handle_data(const QImage&)));

    //ai 결과 signal로 ai 결과 다루는 것 실행
    connect(camera_thread, &CameraThread::send_ai_result, this, &MainWindow::handle_ai_result);

    // 3. 카메라 스레드 실행 (CameraThread::run() 함수가 호출됨)
    camera_thread->start();
    //connect(ui->btnTakeAPicture,&QPushButton::clicked,this,&MainWindow::capture_image);

    // db연결
    if(carDB.connectDB())
        {
            qDebug() << "MainWindow : DB 연결 성공";
        }
        else
        {
            qDebug() << "MainWindow : DB 연결 실패";
        }

}

// ============================================================================
// MainWindow 소멸자 (메모리 해제)
// ============================================================================
MainWindow::~MainWindow() {
    delete ui; // UI 동적 할당 메모리 해제
}

// ============================================================================
// 실시간 이미지 출력 슬롯 함수 (CameraThread에서 시그널을 보낼 때마다 실행)
// ============================================================================
void MainWindow::handle_data(const QImage &image) {
    current_image = image;
    // 1. 스레드로부터 받은 QImage를 화면 출력용 클래스인 QPixmap으로 변환
    QPixmap pixmap = QPixmap::fromImage(image);

    // 2. 영상을 출력할 라벨(lblImg)이 안전하게 존재한다면 이미지를 라벨에 셋팅
    if (ui->lblImg) {
        ui->lblImg->setPixmap(pixmap);
    }

}
//한장씩 찍고 있는 것

// ============================================================================
// 윈도우 창 닫기 이벤트 핸들러 (X 버튼을 누르거나 프로그램을 종료할 때 실행)
// ============================================================================
void MainWindow::closeEvent(QCloseEvent *event) {
    // 카메라 스레드가 동작 중이라면 안전하게 종료 절차를 밟음
    if (camera_thread) {
        camera_thread->quit(); // 1. 스레드의 이벤트 루프(exec())를 빠져나오도록 종료 신호를 보냄
        camera_thread->wait(); // 2. 스레드가 완전히 종료(run() 함수가 완전히 끝날 때)될 때까지 메인 스레드가 대기
    }
    
    // 창 닫기 이벤트를 수락하여 프로그램을 최종적으로 종료시킴
    event->accept();
}

void MainWindow::on_btnTakeAPicture_clicked()
{
    if (current_image.isNull())
    {
        qDebug() << "현재 카메라 이미지 없음";
        return;
    }
    QPixmap pixmap = QPixmap::fromImage(current_image);
    ui->lblPicture->setPixmap(pixmap);

    if (processingDialog)
    {
        processingDialog->close();
        delete processingDialog;
        processingDialog = nullptr;
    }

    processingDialog = new ProcessingDialog(this);
    processingDialog->show();

}

void MainWindow::handle_ai_result(const QString& carnum){
    qDebug() << "AI 번호판 결과 : " << carnum;
    
    if (processingDialog) {
        processingDialog->close();
        delete processingDialog;
        processingDialog = nullptr;
    }

    if (carnum.isEmpty()){
        qDebug() << "번호판 인식 실패";
        ui->lblResult->setText("번호판 인식 실패");
        return;
    }
    ui->lblResult->setText(carnum);

    QString path = "D:\\project\\QT-GUI\\Qt_PyQt_Lab\\final_project_c++_ver\\test_file";
    QDir dir(path);

    if (!dir.exists()) dir.mkpath(".");
    QString filename = QString("%1\\%2.png").arg(path).arg(carnum);

    if (current_image.save(filename))  qDebug() << "번호판 사진 저장 성공:" << filename;
    else qDebug() << "번호판 사진 저장 실패";

    if (carDB.is_car_parked(carnum)){
        qDebug() << carnum << "현재 주차 중 -> 출차";

        ExitDialog dlg(carnum,this);
        if (dlg.exec() == QDialog::Accepted){
            if (carDB.car_out(carnum)){
                qDebug()<< "출차 처리 성공";
                ui->lblResult->setText(carnum+ " 출차 완료");
            }else{
                qDebug() << "출차 처리 실패";
            }
        }
    }else{
        qDebug() << carnum << "현재 주차 안 됨 -> 입차";
        EnterDialog dlg(carnum,this);

        if (dlg.exec() == QDialog::Accepted){
            if (carDB.car_in(carnum)){
                qDebug() << "입차 처리 성공";
                ui->lblResult->setText(carnum + " 입차 완료");
            }else{
                qDebug()<< "입차 처리 실패";
            }
        }
    }
}