# AI 기반 차량 번호 인식 주차 관리 시스템

카메라 영상에서 차량 번호 영역을 추출하고 숫자 4자리를 인식하여 차량의
입·출차를 관리하는 프로젝트입니다.

AI 모델의 **학습은 Python/TensorFlow에서 수행**하고, 실제 Qt
프로그램에서의 **AI 추론은 C++ OpenCV DNN을 통해 수행**하도록
구성했습니다.

------------------------------------------------------------------------

## 주요 기능

-   Qt `QCamera` 기반 실시간 카메라 영상 출력
-   차량 번호 인식을 위한 ROI 설정
-   OpenCV 기반 이미지 전처리
-   번호판 영역의 숫자 4자리 분리
-   Python/TensorFlow 기반 CNN 숫자 분류 모델 학습
-   H5 모델 → ONNX 변환
-   C++ OpenCV DNN 기반 ONNX 추론
-   인식된 차량 번호를 이용한 이미지 저장
-   SQLite 기반 차량 입·출차 관리
-   차량 번호에 따른 입차 / 출차 자동 판단

------------------------------------------------------------------------

# System Architecture

``` text
[ Python - Model Training ]

MNIST Dataset
      ↓
TensorFlow / Keras
      ↓
CNN Model Training
      ↓
mnist_model.h5
      ↓
ONNX Conversion
      ↓
mnist_model.onnx

===============================

[ C++ / Qt - Model Inference ]

Qt QCamera
      ↓
Camera Frame
      ↓
ROI Crop
      ↓
OpenCV Image Processing
      ↓
4 Digit Segmentation
      ↓
OpenCV DNN
      ↓
ONNX Model Inference
      ↓
Car Number "1234"
      ↓
Image Save / SQLite DB
      ↓
Entry / Exit
```

------------------------------------------------------------------------

# 1. Python - CNN Model Training

## 역할

Python에서는 차량 번호의 숫자를 인식하기 위한 **CNN 모델의 학습**을
담당합니다.

여기서 **CNN(Convolutional Neural Network)** 은 숫자 이미지를 학습하고
분류하는 신경망의 구조를 의미합니다.

### 사용 기술

-   Python
-   TensorFlow / Keras
-   MNIST
-   CNN
-   H5
-   ONNX

------------------------------------------------------------------------

## CNN Model

입력된 `28 × 28` 흑백 숫자 이미지를 이용하여 `0 ~ 9`를 분류합니다.

``` python
model = Sequential([
    Conv2D(
        32,
        kernel_size=(3, 3),
        input_shape=(28, 28, 1),
        padding='same',
        activation='relu'
    ),

    MaxPooling2D(
        pool_size=(2, 2),
        strides=2
    ),

    Conv2D(
        64,
        kernel_size=(3, 3),
        padding='same',
        activation='relu'
    ),

    MaxPooling2D(
        pool_size=(2, 2),
        strides=2
    ),

    Flatten(),

    Dense(
        128,
        activation='relu'
    ),

    Dense(
        10,
        activation='softmax'
    )
])
```

## CNN Architecture

``` text
Input
28 × 28 × 1
     ↓
Conv2D
32 filters / 3×3 / ReLU
     ↓
MaxPooling2D
2×2
     ↓
Conv2D
64 filters / 3×3 / ReLU
     ↓
MaxPooling2D
2×2
     ↓
Flatten
     ↓
Dense
128 / ReLU
     ↓
Dense
10 / Softmax
     ↓
0 ~ 9
```

### Input / Output

``` text
Input Shape  : (1, 28, 28, 1)
Output Shape : (1, 10)
```

출력은 숫자 `0~9`에 대한 확률이며 가장 높은 확률을 가진 인덱스를 최종
숫자로 판단합니다.

------------------------------------------------------------------------

## H5 Model

Python에서 학습이 완료된 CNN 모델은 H5 형식으로 저장합니다.

``` text
MNIST
   ↓
CNN Training
   ↓
mnist_model.h5
```

`mnist_model.h5`에는 학습된 모델 구조와 Weight가 저장됩니다.

------------------------------------------------------------------------

## H5 → ONNX

학습된 모델을 C++ 환경에서 사용할 수 있도록 ONNX 형식으로 변환합니다.

``` text
mnist_model.h5
       ↓
ONNX Conversion
       ↓
mnist_model.onnx
```

최종적으로 생성된 `mnist_model.onnx` 파일을 Qt/C++ 프로그램의 OpenCV
DNN에서 사용합니다.

------------------------------------------------------------------------

# 2. Python → C++ Integration

AI 모델의 **학습 및 모델 생성은 Python**, 실제 주차 관리 프로그램에서의
**영상 처리와 추론은 C++**에서 수행하도록 역할을 분리했습니다.

``` text
┌──────────────────────────────┐
│            Python            │
│                              │
│  MNIST Dataset               │
│       ↓                      │
│  TensorFlow / Keras          │
│       ↓                      │
│  CNN Model Training          │
│       ↓                      │
│  mnist_model.h5              │
│       ↓                      │
│  ONNX Conversion             │
└──────────────┬───────────────┘
               │
               │ mnist_model.onnx
               ▼
┌──────────────────────────────┐
│           C++ / Qt           │
│                              │
│  QCamera                     │
│       ↓                      │
│  OpenCV Preprocessing        │
│       ↓                      │
│  OpenCV DNN                  │
│       ↓                      │
│  ONNX Inference              │
│       ↓                      │
│  Car Number                  │
│       ↓                      │
│  SQLite / Entry / Exit       │
└──────────────────────────────┘
```

  구분                  Python               C++
  --------------------- -------------------- ------------------------
  주요 역할             AI 모델 학습         AI 추론 및 시스템 제어
  AI 관련 기술          CNN                  OpenCV DNN
  Framework / Library   TensorFlow / Keras   OpenCV
  Model Format          H5 → ONNX            ONNX
  Input                 MNIST 숫자 이미지    카메라 숫자 이미지
  Output                학습된 모델          차량 번호 4자리
  GUI                   \-                   Qt
  Database              \-                   SQLite

> **CNN**은 Python에서 학습한 신경망의 구조이며, **OpenCV DNN**은 학습된
> ONNX 모델을 C++에서 실행하기 위한 추론 모듈입니다.

------------------------------------------------------------------------

# 3. C++ / Qt - OpenCV DNN Inference

## 역할

C++에서는 CNN 모델을 다시 학습하지 않습니다.

Python에서 학습하고 ONNX로 변환한 모델을 불러와 **OpenCV DNN을 이용해
추론(Inference)** 합니다.

### 사용 기술

-   C++17
-   Qt 5
-   QCamera / QVideoFrame
-   OpenCV
-   OpenCV DNN
-   ONNX
-   SQLite

------------------------------------------------------------------------

## Qt Camera

Qt의 `QCamera`를 이용하여 실시간 카메라 영상을 입력받습니다.

카메라 프레임은 `QVideoFrame`으로 전달되고 `CameraThread`에서 `QImage`로
변환하여 MainWindow로 전달합니다.

``` cpp
emit send_image(flipped_img);
```

MainWindow에서는 전달받은 이미지를 화면에 출력하고 현재 프레임을
보관합니다.

``` cpp
void MainWindow::handle_data(const QImage &image)
{
    current_image = image;

    QPixmap pixmap =
        QPixmap::fromImage(image);

    ui->lblImg->setPixmap(pixmap);
}
```

------------------------------------------------------------------------

# OpenCV Image Processing

카메라 전체 영상을 바로 AI 모델에 입력하지 않고 차량 번호가 위치하는 ROI
영역을 먼저 추출합니다.

``` text
Camera Frame
      ↓
300 × 120 ROI
      ↓
Grayscale
      ↓
Gaussian Blur
      ↓
Binary Threshold
      ↓
Contour Detection
      ↓
Digit Candidate Filtering
      ↓
4 Digit Segmentation
```

## Grayscale

``` cpp
cv::cvtColor(
    rgb,
    gray,
    cv::COLOR_RGB2GRAY
);
```

컬러 영상을 Grayscale로 변환하여 숫자와 배경을 구분하기 위한 전처리를
수행합니다.

## Gaussian Blur

``` cpp
cv::GaussianBlur(
    gray,
    gray,
    cv::Size(5, 5),
    0
);
```

작은 노이즈를 제거하여 Threshold 및 Contour 검출을 안정화합니다.

## Binary Threshold

``` cpp
cv::threshold(
    gray,
    binary,
    0,
    255,
    cv::THRESH_BINARY_INV | cv::THRESH_OTSU
);
```

Otsu Threshold를 적용하여 숫자와 배경을 분리합니다.

``` text
Background → Black
Digit      → White
```

## Contour Detection

``` cpp
cv::findContours(
    binary,
    contours,
    cv::RETR_EXTERNAL,
    cv::CHAIN_APPROX_SIMPLE
);
```

이진화된 이미지에서 Contour를 검출하여 숫자 후보 영역을 찾습니다.

------------------------------------------------------------------------

# 4-Digit Segmentation

학습된 CNN 모델은 한 번에 숫자 하나를 분류하므로 번호판의 숫자 4개를
각각 분리합니다.

``` text
┌─────────────────────┐
│     1  2  3  4      │
└─────────────────────┘
          ↓
┌───┐ ┌───┐ ┌───┐ ┌───┐
│ 1 │ │ 2 │ │ 3 │ │ 4 │
└───┘ └───┘ └───┘ └───┘
  ↓     ↓     ↓     ↓
28×28 28×28 28×28 28×28
```

검출된 숫자 영역은 X 좌표를 기준으로 왼쪽에서 오른쪽 순서로 정렬합니다.

``` cpp
std::sort(
    digit_boxes.begin(),
    digit_boxes.end(),
    [](const cv::Rect &a, const cv::Rect &b)
    {
        return a.x < b.x;
    }
);
```

------------------------------------------------------------------------

# AI Input Preprocessing

분리된 숫자는 Python에서 학습한 CNN 모델의 입력과 동일한 형태로
변환합니다.

``` text
Digit Image
     ↓
Padding
     ↓
28 × 28 Resize
     ↓
Normalization
     ↓
Model Input
```

픽셀 값은 `0.0 ~ 1.0` 범위로 정규화합니다.

``` cpp
digit.convertTo(
    digit,
    CV_32F,
    1.0 / 255.0
);
```

``` text
0 ~ 255 → 0.0 ~ 1.0
```

------------------------------------------------------------------------

# OpenCV DNN

## ONNX Model Loading

C++에서는 OpenCV의 `dnn` 모듈을 사용하여 Python에서 생성한 ONNX 모델을
로딩합니다.

``` cpp
#include <opencv2/dnn.hpp>
```

``` cpp
m_model =
    cv::dnn::readNetFromONNX(
        model_path.toStdString()
    );
```

``` text
Python CNN Model
       ↓
      H5
       ↓
     ONNX
       ↓
C++ OpenCV DNN
```

------------------------------------------------------------------------

## DNN Input

Python 모델과 동일한 입력 형태 `(1, 28, 28, 1)`를 생성합니다.

``` cpp
int sizes[] =
{
    1,
    28,
    28,
    1
};

cv::Mat input(
    4,
    sizes,
    CV_32F
);
```

``` text
Batch   : 1
Height  : 28
Width   : 28
Channel : 1
```

------------------------------------------------------------------------

## DNN Inference

전처리된 숫자를 OpenCV DNN에 입력합니다.

``` cpp
m_model.setInput(input);

cv::Mat output =
    m_model.forward();
```

`forward()`를 통해 ONNX 모델의 추론을 수행합니다.

출력값 중 가장 높은 확률의 인덱스를 숫자로 결정합니다.

``` cpp
cv::minMaxLoc(
    prediction,
    nullptr,
    &max_value,
    nullptr,
    &max_loc
);

int number = max_loc.x;
```

각 숫자에 대해 동일한 추론을 수행한 뒤 결과를 순서대로 연결합니다.

``` text
[1] → DNN → 1
[2] → DNN → 2
[3] → DNN → 3
[4] → DNN → 4

1 + 2 + 3 + 4
      ↓
    "1234"
```

------------------------------------------------------------------------

# Qt Signal-Slot

AI 인식이 완료되면 `CameraThread`에서 인식된 차량 번호를 MainWindow로
전달합니다.

``` cpp
emit send_ai_result(car_number);
```

Signal-Slot 연결:

``` cpp
connect(
    camera_thread,
    &CameraThread::send_ai_result,
    this,
    &MainWindow::handle_ai_result
);
```

``` text
CameraThread
     ↓
OpenCV DNN
     ↓
"1234"
     ↓
send_ai_result()
     ↓
MainWindow
     ↓
handle_ai_result()
```

------------------------------------------------------------------------

# Image Save

AI 인식이 완료된 후 인식된 차량 번호를 파일명에 사용하여 촬영 이미지를
저장합니다.

``` text
AI Recognition
      ↓
    "1234"
      ↓
Image Save
      ↓
1234_20260819_220401.jpg
```

차량 번호와 촬영 시간을 함께 사용하여 동일 차량의 이미지가 기존 파일을
덮어쓰지 않도록 구성할 수 있습니다.

------------------------------------------------------------------------

# Database

차량의 입·출차 정보는 **SQLite**를 이용하여 관리합니다.

``` sql
CREATE TABLE IF NOT EXISTS parking (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    car_number TEXT UNIQUE,
    in_time TEXT,
    is_deleted BOOLEAN DEFAULT 0 CHECK(is_deleted IN (0,1))
);
```

## Table Structure

  Column         Description
  -------------- -----------------------
  `id`           차량 데이터 식별 번호
  `car_number`   AI로 인식한 차량 번호
  `in_time`      차량 입차 시간
  `is_deleted`   현재 주차 여부

주차 상태는 `is_deleted` 값을 이용하여 관리합니다.

``` text
is_deleted = 0 → 현재 주차 중
is_deleted = 1 → 출차 완료
```

AI에서 차량 번호가 인식되면 해당 번호를 이용해 DB를 조회합니다.

``` text
OpenCV DNN
     ↓
AI Result
   "1234"
     ↓
SQLite Search
     ↓
is_car_parked("1234")
     ↓
현재 주차 중?
   /       \
 NO         YES
 │           │
 ▼           ▼
Entry       Exit
 │           │
 ▼           ▼
car_in()   car_out()
```

## Entry

DB에 현재 주차 중인 동일 차량이 존재하지 않으면 입차 차량으로
판단합니다.

``` cpp
if(!carDB.is_car_parked(carNumber))
{
    carDB.car_in(carNumber);
}
```

``` text
AI Result : 1234
     ↓
DB Search
     ↓
현재 주차 X
     ↓
Entry Dialog
     ↓
car_in("1234")
     ↓
DB 저장
```

입차 시 차량 번호와 입차 시간을 저장하고 `is_deleted = 0` 상태로
관리합니다.

## Exit

동일한 차량 번호가 `is_deleted = 0` 상태로 존재하면 현재 주차 중인
차량으로 판단합니다.

``` cpp
if(carDB.is_car_parked(carNumber))
{
    carDB.car_out(carNumber);
}
```

``` text
AI Result : 1234
     ↓
DB Search
     ↓
현재 주차 O
     ↓
Exit Dialog
     ↓
car_out("1234")
     ↓
is_deleted = 1
```
------------------------------------------------------------------------

# Project Structure

``` text
project/
│
├── pythonQT/
│   ├── main.py
│   ├── gui.py
│   ├── Thread.py
│   ├── mnist_model.h5
│   └── mnist_model.onnx  
│
├── Qt_Cpp/
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── mainwindow.h
│   ├── camerathread.cpp
│   ├── camerathread.h
│   ├── cardbmanager.cpp
│   ├── cardbmanager.h
│   ├── mainwindow.ui
│   └── mnist_model.onnx 
│
└── README.md
```

---

# 실행 화면

<p align="center">
  <img src=".\result.png" width="400" alt="차량 번호 AI 인식 실행 화면">
</p>

