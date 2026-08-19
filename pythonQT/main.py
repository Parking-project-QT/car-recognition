import sys
import subprocess

from PyQt5.QtWidgets import (
    QApplication,
    QMainWindow
)

from PyQt5.QtGui import (
    QPixmap,
    QImage
)

from PyQt5.QtCore import (
    pyqtSlot,
    Qt
)


# ============================================================
# UI 파일 이름
# ============================================================
GUI_FILE_NAME = 'gui'


# ============================================================
# gui.ui -> gui.py 변환
# ============================================================
subprocess.run([
    sys.executable,
    '-m',
    'PyQt5.uic.pyuic',
    '-x',
    f'{GUI_FILE_NAME}.ui',
    '-o',
    f'{GUI_FILE_NAME}.py'
])


# ============================================================
# UI Import
# ============================================================
from gui import Ui_MainWindow

from Thread import CameraThread


# ============================================================
# Main Window
# ============================================================
class Form(
    QMainWindow,
    Ui_MainWindow
):

    def __init__(self):

        super().__init__()


        # =====================================================
        # UI 초기화
        # =====================================================
        self.setupUi(
            self
        )


        # =====================================================
        # 카메라 Thread 생성
        # =====================================================
        self.camera_thread = CameraThread(
            self
        )


        # =====================================================
        # 카메라 영상 signal
        # =====================================================
        self.camera_thread.send_image.connect(
            self.handle_data
        )


        # =====================================================
        # AI 결과 signal
        # =====================================================
        self.camera_thread.send_ai_result.connect(
            self.handle_ai_result
        )


        # =====================================================
        # 인식 버튼
        #
        # Designer 버튼 이름:
        #
        # btnRecognition
        # =====================================================
        self.btnRecognition.clicked.connect(
            self.camera_thread.request_ai
        )


        # =====================================================
        # 이미지 중앙 정렬
        # =====================================================
        self.lblImg.setAlignment(
            Qt.AlignCenter
        )


        # =====================================================
        # Thread 시작
        # =====================================================
        self.camera_thread.start()


    # ============================================================
    # 카메라 영상 출력
    # ============================================================
    @pyqtSlot(QImage)
    def handle_data(
        self,
        image
    ):

        pixmap = QPixmap.fromImage(
            image
        )


        # ========================================================
        # QLabel 크기에 맞게 조절
        #
        # 비율 유지
        # ========================================================
        pixmap = pixmap.scaled(
            self.lblImg.size(),
            Qt.KeepAspectRatio,
            Qt.SmoothTransformation
        )


        self.lblImg.setPixmap(
            pixmap
        )


    # ============================================================
    # AI 결과
    # ============================================================
    @pyqtSlot(str)
    def handle_ai_result(
        self,
        result
    ):

        print(
            "Main AI Result:",
            result
        )


        self.lblResult.setText(
            result
        )


    # ============================================================
    # 프로그램 종료
    # ============================================================
    def closeEvent(
        self,
        event
    ):

        # Thread Event Loop 종료
        self.camera_thread.quit()


        # Thread 종료까지 대기
        self.camera_thread.wait()


        event.accept()


# ============================================================
# Main
# ============================================================
if __name__ == '__main__':

    app = QApplication(
        sys.argv
    )


    w = Form()


    w.show()


    sys.exit(
        app.exec()
    )