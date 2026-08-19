import os
import cv2
import numpy as np

from PyQt5.QtCore import QThread, pyqtSignal
from PyQt5.QtMultimedia import (QCamera,QCameraInfo,QAbstractVideoSurface,QAbstractVideoBuffer,QVideoFrame)
from PyQt5.QtGui import QImage, QPainter, QPen
from PyQt5.QtCore import Qt

# ============================================================
# 카메라 프레임을 전달받기 위한 Surface
# ============================================================
class VideoSurface(QAbstractVideoSurface):
    def __init__(self, callback, parent=None):
        super().__init__(parent)
        self.callback = callback
    def supportedPixelFormats(self, handleType):
        return [QVideoFrame.Format_RGB32,QVideoFrame.Format_ARGB32]

    def present(self, frame):
        if frame.isValid():
            self.callback(frame)
        return True

# ============================================================
# Camera Thread
# ============================================================
class CameraThread(QThread):
    # 카메라 화면 전송
    send_image = pyqtSignal(QImage)
    # AI 결과 전송
    send_ai_result = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        # =====================================================
        # Camera
        # =====================================================
        self.camera = None
        self.video_surface = None

        # =====================================================
        # AI Model
        # =====================================================
        self.model = None

        # =====================================================
        # 현재 카메라 프레임 저장
        #
        # 버튼 눌렀을 때 이 이미지를 사용
        # =====================================================
        self.current_image = None

        # =====================================================
        # ROI
        #
        # 화면 중앙의 번호판 인식 영역 크기
        # =====================================================
        self.roi_width = 300
        self.roi_height = 120

    def make_square_digit(self, digit):
        h, w = digit.shape
        # 숫자 주변 여백
        padding = 10

        size = max(h,w) + padding * 2

        # 검은 배경 생성
        square = np.zeros((size, size),dtype=np.uint8)
        # 가운데 위치
        x = (size - w) // 2
        y = (size - h) // 2
        square[y:y + h,x:x + w] = digit

        return square


    # ============================================================
    # Thread 실행
    # ============================================================
    def run(self):

        # AI 모델 로딩
        if self.init_ai() < 0:

            print("AI 모델 로딩 실패")

            return


        # Camera 초기화
        if self.init_capture() < 0:

            return


        # Camera 시작
        if self.start_capture() < 0:

            return


        # Thread Event Loop
        self.exec_()


        # 프로그램 종료
        self.stop_capture()

        self.close_capture()


    # ============================================================
    # AI 모델 초기화
    # ============================================================
    def init_ai(self):

        try:

            print("ONNX AI Model Loading...")


            # Thread.py가 있는 폴더
            base_dir = os.path.dirname(
                os.path.abspath(__file__)
            )


            model_path = os.path.join(
                base_dir,
                "mnist_model.onnx"
            )


            if not os.path.exists(model_path):

                print(
                    "ONNX 파일이 없습니다:",
                    model_path
                )

                return -1


            # ONNX 모델 로딩
            self.model = cv2.dnn.readNetFromONNX(
                model_path
            )


            print("ONNX AI Model Loading 완료")

            return 0


        except Exception as e:

            print(
                "ONNX 모델 로딩 실패:",
                e
            )

            return -1


    # ============================================================
    # Camera 초기화
    # ============================================================
    def init_capture(self):

        if self.camera:

            return 0


        cameras = QCameraInfo.availableCameras()


        if not cameras:

            print(
                "연결된 카메라 장치가 없습니다!"
            )

            return -1


        print(
            "=== 발견된 카메라 장치 목록 ==="
        )


        for i, cam in enumerate(cameras):

            print(
                f"Index [{i}] : "
                f"{cam.description()}"
            )


        print(
            "==============================="
        )


        # ========================================================
        # USB CAMERA 선택
        #
        # 현재 PC에서는 index 2
        # ========================================================
        target_index = 2 if len(cameras) > 2 else 0


        print(
            "현재 선택된 카메라:",
            cameras[target_index].description()
        )


        self.camera = QCamera(
            cameras[target_index]
        )


        self.video_surface = VideoSurface(
            self.process_video_frame
        )


        self.camera.setViewfinder(
            self.video_surface
        )


        return 0


    # ============================================================
    # 카메라 프레임 처리
    # ============================================================
    def process_video_frame(self, frame):

        if not frame.isValid():

            return


        clone_frame = QVideoFrame(
            frame
        )


        # 읽기 모드로 Frame map
        if clone_frame.map(
            QAbstractVideoBuffer.ReadOnly
        ):

            try:

                width = clone_frame.width()

                height = clone_frame.height()

                bytes_per_line = (
                    clone_frame.bytesPerLine()
                )


                ptr = clone_frame.bits()


                if ptr is None:

                    return


                size = (
                    bytes_per_line
                    * height
                )


                ptr.setsize(size)


                # =================================================
                # QVideoFrame -> QImage
                # =================================================
                image = QImage(
                    ptr,
                    width,
                    height,
                    bytes_per_line,
                    QImage.Format_RGB32
                )


                if image.isNull():

                    return


                # =================================================
                # 중요
                #
                # QVideoFrame 메모리와 분리
                # =================================================
                image = image.copy()


                # =================================================
                # 표준 이미지 포맷으로 변경
                # =================================================
                image = image.convertToFormat(
                    QImage.Format_RGBA8888
                )


                # =================================================
                # 현재 카메라가 뒤집혀 있다면 반전
                # =================================================
                image = image.mirrored(
                    False,
                    True
                )


                # =================================================
                # 원본 이미지 저장
                #
                # ROI 네모가 그려지기 전 이미지
                # AI에서는 이것을 사용
                # =================================================
                self.current_image = image.copy()


                # =================================================
                # 화면 출력용 복사
                # =================================================
                display_image = image.copy()


                # =================================================
                # ROI 네모 그리기
                # =================================================
                self.draw_roi(
                    display_image
                )


                # =================================================
                # MainWindow로 전달
                # =================================================
                self.send_image.emit(
                    display_image
                )


            except Exception as e:

                print(
                    "프레임 변환 오류:",
                    e
                )


            finally:

                clone_frame.unmap()


    # ============================================================
    # 화면 중앙에 ROI 네모 그리기
    # ============================================================
    def draw_roi(self, image):

        width = image.width()

        height = image.height()


        # ========================================================
        # 중앙 좌표 계산
        # ========================================================
        x = (
            width
            - self.roi_width
        ) // 2


        y = (
            height
            - self.roi_height
        ) // 2


        painter = QPainter(
            image
        )


        pen = QPen(
            Qt.red
        )


        pen.setWidth(
            3
        )


        painter.setPen(
            pen
        )


        # ========================================================
        # 네모 영역
        # ========================================================
        painter.drawRect(
            x,
            y,
            self.roi_width,
            self.roi_height
        )


        painter.end()


    # ============================================================
    # 버튼 눌렀을 때 호출
    # ============================================================
    def request_ai(self):

        if self.current_image is None:

            print(
                "현재 카메라 이미지가 없습니다."
            )

            return


        print(
            "================================"
        )

        print(
            "AI 인식 요청"
        )


        # ========================================================
        # 버튼을 누른 순간의 이미지 복사
        # ========================================================
        image = self.current_image.copy()


        # AI 실행
        self.process_ai(
            image
        )


    # ============================================================
    # ROI 영역 추출
    # ============================================================
    def crop_roi(self, image):

        width = image.width()

        height = image.height()


        x = (
            width
            - self.roi_width
        ) // 2


        y = (
            height
            - self.roi_height
        ) // 2


        # ========================================================
        # 중앙 네모 영역만 자르기
        # ========================================================
        roi_image = image.copy(
            x,
            y,
            self.roi_width,
            self.roi_height
        )


        return roi_image


    # ============================================================
    # AI 처리
    #
    # 현재 단계:
    #
    # ROI 전체를 28 x 28로 변환해서
    # 숫자 하나를 추론
    #
    # 이후:
    # ROI 내부에서 숫자 4개 분리 필요
    # ============================================================
    def process_ai(self, image):

        if self.model is None:
            return

        try:
            print("================================")
            print("AI 인식 요청")

            # =====================================================
            # 1. ROI 자르기
            # =====================================================
            roi_image = self.crop_roi(image)

            print(
                "ROI Size:",
                roi_image.width(),
                "x",
                roi_image.height()
            )

            # =====================================================
            # 2. QImage -> RGB
            # =====================================================
            rgb_image = roi_image.convertToFormat(
                QImage.Format_RGB888
            )

            width = rgb_image.width()
            height = rgb_image.height()
            bytes_per_line = rgb_image.bytesPerLine()

            ptr = rgb_image.bits()

            ptr.setsize(
                bytes_per_line * height
            )

            array = np.frombuffer(
                ptr,
                dtype=np.uint8
            )

            array = array.reshape(
                height,
                bytes_per_line
            )

            array = array[:, :width * 3]

            array = array.reshape(
                height,
                width,
                3
            ).copy()

            # =====================================================
            # 3. Gray
            # =====================================================
            gray = cv2.cvtColor(
                array,
                cv2.COLOR_RGB2GRAY
            )

            # =====================================================
            # 4. Blur
            # =====================================================
            gray = cv2.GaussianBlur(
                gray,
                (5, 5),
                0
            )

            # =====================================================
            # 5. Threshold
            #
            # 흰 배경 + 검은 숫자
            #       ↓
            # 검은 배경 + 흰 숫자
            # =====================================================
            _, binary = cv2.threshold(
                gray,
                0,
                255,
                cv2.THRESH_BINARY_INV
                + cv2.THRESH_OTSU
            )

            # =====================================================
            # 6. Contour 찾기
            # =====================================================
            contours, _ = cv2.findContours(
                binary,
                cv2.RETR_EXTERNAL,
                cv2.CHAIN_APPROX_SIMPLE
            )

            digit_boxes = []

            roi_h, roi_w = binary.shape

            # =====================================================
            # 7. 숫자로 보이는 contour만 선택
            # =====================================================
            for contour in contours:

                x, y, w, h = cv2.boundingRect(
                    contour
                )

                # 너무 작은 노이즈 제거
                if w < 10:
                    continue

                if h < 30:
                    continue

                # 너무 큰 영역 제거
                if w > roi_w * 0.5:
                    continue

                if h > roi_h * 0.95:
                    continue

                digit_boxes.append(
                    (x, y, w, h)
                )

            print(
                "찾은 숫자 후보:",
                digit_boxes
            )

            # =====================================================
            # 8. 왼쪽 → 오른쪽 정렬
            # =====================================================
            digit_boxes.sort(
                key=lambda box: box[0]
            )

            # =====================================================
            # 정확히 4개가 아니면 실패
            # =====================================================
            if len(digit_boxes) != 4:
                result = (
                    f"숫자 4개를 찾지 못했습니다 "
                    f"({len(digit_boxes)}개)"
                )

                print(result)

                self.send_ai_result.emit(
                    result
                )

                return

            # =====================================================
            # 9. 각 숫자 AI 인식
            # =====================================================
            car_number = ""

            confidences = []

            for i, (x, y, w, h) in enumerate(
                    digit_boxes
            ):
                # ---------------------------------------------
                # 숫자 crop
                # ---------------------------------------------
                digit = binary[
                        y:y + h,
                        x:x + w
                        ]

                # ---------------------------------------------
                # 정사각형 padding
                #
                # 숫자를 그냥 28x28로 찌그러뜨리지 않기 위해
                # ---------------------------------------------
                digit = self.make_square_digit(
                    digit
                )

                # ---------------------------------------------
                # 28 x 28
                # ---------------------------------------------
                digit = cv2.resize(
                    digit,
                    (28, 28),
                    interpolation=cv2.INTER_AREA
                )

                # ---------------------------------------------
                # float
                # ---------------------------------------------
                digit = digit.astype(
                    np.float32
                )

                digit /= 255.0

                # ---------------------------------------------
                # 입력 형태
                #
                # (1,28,28,1)
                # ---------------------------------------------
                input_data = digit.reshape(
                    1,
                    28,
                    28,
                    1
                )

                # ---------------------------------------------
                # AI
                # ---------------------------------------------
                self.model.setInput(
                    input_data
                )

                output = self.model.forward()

                prediction = output.flatten()

                number = int(
                    np.argmax(
                        prediction
                    )
                )

                confidence = float(
                    prediction[number]
                )

                # ---------------------------------------------
                # 번호 조합
                # ---------------------------------------------
                car_number += str(
                    number
                )

                confidences.append(
                    confidence
                )

                print(
                    f"{i + 1}번째 숫자:",
                    number,
                    f"{confidence * 100:.1f}%"
                )

            # =====================================================
            # 10. 최종 결과
            # =====================================================
            avg_confidence = (
                    sum(confidences)
                    / len(confidences)
            )

            result = (
                f"{car_number} "
                f"({avg_confidence * 100:.1f}%)"
            )

            print(
                "최종 차량번호:",
                result
            )

            self.send_ai_result.emit(
                result
            )

        except cv2.error as e:

            print(
                "OpenCV 오류:",
                e
            )

        except Exception as e:

            print(
                "AI 처리 오류:",
                e
            )


    # ============================================================
    # Camera 시작
    # ============================================================
    def start_capture(self):

        if self.camera:

            self.camera.start()


            print(
                "PyQt5 QCamera Stream on..."
            )


            return 0


        return -1


    # ============================================================
    # Camera 종료
    # ============================================================
    def stop_capture(self):

        if self.camera:

            self.camera.stop()


            print(
                "PyQt5 QCamera Stream off!!"
            )


            return 0


        return -1


    # ============================================================
    # Camera 객체 제거
    # ============================================================
    def close_capture(self):

        self.camera = None

        self.video_surface = None