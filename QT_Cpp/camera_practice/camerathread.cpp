#include "camerathread.h"
#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QCoreApplication>

#include <algorithm> //sort 사용
#include <cstring>

// ============================================================================
// CameraThread 생성자 (멤버 변수 안전 초기화)
// ============================================================================
CameraThread::CameraThread(QObject *parent)
    : QThread(parent)
    , m_camera(nullptr)
    , m_video_surface(nullptr)
    , m_frame_count(0)
    , m_frame_divisor(1)
    , m_roi_width(300)
    , m_roi_height(120)
{
}

// ============================================================================
// CameraThread 소멸자 (스레드 안전 종료 보장)
// ============================================================================
CameraThread::~CameraThread() {
    // 스레드가 아직 실행 중이라면 run() 함수가 완전히 끝날 때까지 대기하여 안전한 파괴를 보장
    quit(); //이벤트 루프를 끝내기 위한 요청 ->exec()를 끝내기 위해서
    wait(); //실제로 스레드가 다 끝날 때까지 기다림
}

// ============================================================================
// 스레드 메인 실행 함수 (start() 호출 시 백그라운드에서 자동 실행)
// ============================================================================
void CameraThread::run() {
    if (init_ai() < 0) return; //ai 모델 초기화 실패시 즉시 종료
    // 1. 카메라 장치 및 수신기 초기화 (실패 시 스레드 즉시 종료)
    if (init_capture() < 0) return;
    
    // 2. 카메라 영상 스트리밍 시작 (실패 시 스레드 즉시 종료)
    if (start_capture() < 0) return;

    // 3. 비동기 이벤트 루프 구동 (quit() 신호가 올 때까지 여기서 대기하며 이벤트 처리)
    exec();

    // 4. 이벤트 루프가 종료되면 카메라를 멈추고 자원을 해제
    stop_capture();
    close_capture();
}

// ============================================================================
// 카메라 및 비디오 수신기(Surface) 초기화 함수
// ============================================================================
int CameraThread::init_capture() {
    // 이미 카메라 객체가 생성되어 있다면(중복 초기화 방지) 정상 종료(0) 반환
    if (m_camera) return 0;

    // 시스템(PC)에 사용 가능한 카메라 장치 목록을 불러옴
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
        qDebug() << "No camera detected!";
        return -1;
    }

    // 디버그 콘솔에 발견된 카메라 목록 출력
    qDebug() << "=== Detected Camera List ===";
    for (int i = 0; i < cameras.size(); ++i) {
        qDebug() << "Index [" << i << "] :" << cameras[i].description();
    }
    qDebug() << "===============================";

    // 카메라가 2개 이상이면 1번(외장 카메라), 1개뿐이면 0번(기본 내장 카메라)을 타겟으로 설정
    int target_index = (cameras.size() > 1) ? 1 : 0;
    qDebug() << "Current Camera:" << cameras[target_index].description();

    // 선택한 카메라 정보를 바탕으로 실제 제어할 QCamera 객체를 동적 생성
    m_camera = new QCamera(cameras[target_index]);

    // 영상 수신기(VideoSurface)를 생성하면서, 
    // 새 프레임이 들어올 때마다 클래스 내부 함수(process_video_frame)를 실행하도록 람다식으로 묶어줌
    m_video_surface = new VideoSurface([this](const QVideoFrame& frame) {
        this->process_video_frame(frame);
    });

    // 카메라의 뷰파인더(출력 대상)를 우리가 만든 영상 수신기(m_video_surface)로 지정하여 영상을 그리도록 설정
    m_camera->setViewfinder(m_video_surface);
    return 0;
}

//---AI Model init--//
int CameraThread::init_ai() {
    try {
        //model 실행 위치 지정하기
        QString model_path = QCoreApplication::applicationDirPath() + "/mnist_model.onnx";
        qDebug() << "ONNX Model Path:" << model_path;

        //model dnn으로 생성
        m_model = cv::dnn::readNetFromONNX(model_path.toStdString());

        if (m_model.empty()){
            qDebug() << "ONNX Model Empty";
            return -1;
        }
        qDebug() << "ONNX AI Model Loading Complete";
        return 0;
    }
    //error
    catch (const cv::Exception& e)
    {
        qDebug() << e.what();
        return -1;
    }
}
// ============================================================================
// 수신된 비디오 프레임 가공 및 메인 UI 전송 함수
// ============================================================================
void CameraThread::process_video_frame(const QVideoFrame &frame) {
    // 유효하지 않은 데이터 프레임이라면 즉시 제외
    if (!frame.isValid()) return;

    QVideoFrame clone_frame(frame);

    // CPU가 비디오 데이터 메모리에 접근할 수 있도록 읽기 전용(ReadOnly) 매핑 시도
    if (clone_frame.map(QAbstractVideoBuffer::ReadOnly)) {
        try {
            // 이미지 생성을 위한 프레임 정보 추출
            int width = clone_frame.width();
            int height = clone_frame.height();
            int bytes_per_line = clone_frame.bytesPerLine();
            uchar *ptr = clone_frame.bits();

            // 메모리 주소가 정상이라면
            if (ptr != nullptr) {
                // 원본 메모리 주소를 참조하여 QImage 생성 (복사 없는 빠른 래핑)
                QImage img(ptr, width, height, bytes_per_line, QImage::Format_RGB32);

                if (!img.isNull()) {
                    m_frame_count++; // 프레임 카운트 증가
                    
                    // 정해진 분모(Divisor) 주기에 해당할 때만 가공 및 전송 (프레임 스킵 기법)
                    if (m_frame_count % m_frame_divisor == 0) {
                        
                        // 1. 범용적인 표준 포맷(RGBA8888)으로 변환
                        QImage standard_img = img.convertToFormat(QImage::Format_RGBA8888);

                        // 2. 상하 반전 (카메라 센서 특성 및 그래픽 좌표계 뒤집힘 보정)
                        QImage flipped_img = standard_img.mirrored(false, true);

                        m_current_image = flipped_img.copy();// 현재 프레임 저장
                        QImage display = fliped_img.copy(); //display 이미지 저장용

                        //ROI 중앙 위치 계산
                        int roi_x = (display.width() - m_roi_width) / 2;
                        int roi_y = (display.height() - m_roi_height) / 2;

                        //화면에 ROI 사각형 표시
                        QPainter painter(&display);
                        QPen pen(Qt::green);
                        pen.setWidth(3);
                        painter.setPen(pen);
                        painter.drawRect(roi_x, roi_y, m_roi_width, m_roi_height);
                        painter.end();

                        // 3. 메인 윈도우(GUI)로 완성된 이미지 전송 신호 발생
                        emit send_image(display);
                    }
                }
            }
        } catch (...) {
            // 프레임 변환 과정 중 예외 발생 시 크래시 방지용 예외 처리
            qDebug() << "Frame Conversion Error";
        }
        
        // 메모리 매핑 해제 (자원 누수 방지를 위해 필수)
        clone_frame.unmap();
    }
}
//현재 이미지 저장
bool CameraThread::save_current_image(const QString& path) {
    if (m_current_image.isNull()) return false;
    return m_current_image.save(path);
}
//---AI 요청---//
void CameraThread::request_ai() {
    if (m_current_image.isNull()) {
        qDebug() << "Current Image Empty";
        return;
    }
    process_ai(m_current_image.copy());
}

//QImage를 cv 매트릭스로 변환
cv::Mat CameraThread::qimage_to_mat(const QImage& image) {
    QImage rgb = image.convertToFormat(QImage::Format_RGB888); //이미지 포맷을 rgb형식으로 변환
    // OpenCV의 cv::Mat 객체로 복사 없이(Zero-copy) 래핑
    cv::Mat mat(rgb.height(), rgb.width(), CV_8UC3, const_cast<uchar*>(rgb.bits()), rgb.bytesPerLine());
    return mat.clone();
}
//숫자 padding으로 정사각형 형태로 만드는 것이야
cv::Mat CameraThread::make_square_digit(const cv::Mat& digit) {
    int h = digit.rows;
    int w = digit.cols;
    int padding = 10;
    int size = std::max(h, w) + padding * 2; // 양쪽에 여백 줘 정사각형 크기 결정

    cv::Mat square = cv::Mat::zeros(size, size, CV_8UC1); //숫자가 들어갈 바탕이 될 검은색 정사각형 배경 생성
    int x = (size - w) / 2;
    int y = (size - h) / 2;

    digit.copyTo(square(cv::Rect(x, y, w, h))); //검은색 정사각형 이미지 안에 숫자 이미지 그대로 복사해 넣기

    return square;
}
//---AI 동작---//
void CameraThread::process_ai(const QImage& image){
    try{
        // ROI
        int roi_x = (image.width() - m_roi_width) / 2;
        int roi_y = (image.height()- m_roi_height) / 2;

        QImage roi_image = image.copy(roi_x,roi_y,m_roi_width,m_roi_height);

        // QImage 매트릭스로 변환 
        cv::Mat rgb =qimage_to_mat(roi_image);

        // 색 반전 
        cv::Mat gray;
        cv::cvtColor(rgb,gray,cv::COLOR_RGB2GRAY);

        // Blur : 노이즈 제거를 위해서
        cv::GaussianBlur(gray,gray,cv::Size(5, 5),0);

        // Threshold -> 임계처리를 통해 배경 글자 분리
        cv::Mat binary;
        cv::threshold(gray,binary,0,255,cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

        // Contour 외곽선 검출을 통해 경계선 연결하여 정밀하게 분석 위해서
        std::vector<std::vector<cv::Point> > contours;
        cv::findContours(binary,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
        std::vector<cv::Rect>digit_boxes;

        for (const auto& contour :contours)
        {
            cv::Rect rect = cv::boundingRect(contour);
            // 노이즈 제거
            // 외곽선의 바운딩 박스(boundingRect) 크기를 검사하여
            // '숫자(digit)' 크기의 박스만 골라내고 노이즈나 너무 큰/작은 영역을 걸러내는 필터링(조건문) 과정
            if (rect.width < 10) continue;
            if (rect.height < 30) continue;
            if (rect.width > binary.cols * 0.5) continue;
            if (rect.height > binary.rows * 0.95) continue;

            digit_boxes.push_back(rect);
        }
        // 좌 -> 우 정렬
        std::sort(digit_boxes.begin(), digit_boxes.end(), [](const cv::Rect& a, const cv::Rect& b) {return a.x < b.x;});
        // 4자리가 아닌 경우
        if (digit_boxes.size() != 4){
            emit send_ai_result("");
            return;
        }

        // 숫자 4개 인식
        QString car_number = "";
        for (int i = 0;i < 4;i++){
            cv::Mat digit = binary(digit_boxes[i]).clone();

            digit = make_square_digit(digit);
            cv::resize(digit,digit,cv::Size(28,28),0,0,cv::INTER_AREA);
            digit.convertTo(digit,CV_32F,1.0 / 255.0);

            // ONNX Input = NHWC
            int sizes[] = {1,28,28,1};
            cv::Mat input(4,sizes,CV_32F);
            std::memcpy(input.ptr<float>(),digit.ptr<float>(),28 * 28 * sizeof(float));

            m_model.setInput(input);

            cv::Mat output = m_model.forward();
            cv::Mat prediction = output.reshape(1,1);

            double max_value;
            cv::Point max_loc;
            cv::minMaxLoc(prediction,nullptr,&max_value,nullptr,&max_loc);

            int number = max_loc.x;
            car_number +=QString::number(number);
        }

        // 결과
        qDebug() << "Car Number:" << car_number;
        emit send_ai_result(car_number);
    }

    catch (const cv::Exception& e){
        qDebug()<< e.what();

        emit send_ai_result("");
    }
}
// ============================================================================
// 카메라 스트리밍 시작 함수
// ============================================================================
int CameraThread::start_capture() {
    if (m_camera) {
        m_camera->start(); // 하드웨어 카메라 가동 시작
        qDebug() << "QCamera Stream on...";
        return 0;
    }
    return -1;
}

// ============================================================================
// 카메라 스트리밍 중지 함수
// ============================================================================
void CameraThread::stop_capture() {
    if (m_camera) {
        m_camera->stop(); // 하드웨어 카메라 가동 중지
        qDebug() << "QCamera Stream off!!";
    }
}

// ============================================================================
// 메모리 자원 최종 해제 및 초기화 함수
// ============================================================================
void CameraThread::close_capture() {
    // 1. 동적 할당된 카메라 객체 메모리 해제 및 포인터 초기화
    if (m_camera) {
        delete m_camera;
        m_camera = nullptr;
    }
    
    // 2. 동적 할당된 비디오 수신기 객체 메모리 해제 및 포인터 초기화
    if (m_video_surface) {
        delete m_video_surface;
        m_video_surface = nullptr;
    }
}