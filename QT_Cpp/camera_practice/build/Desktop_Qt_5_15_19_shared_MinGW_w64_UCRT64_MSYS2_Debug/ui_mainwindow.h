/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.19
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QLabel *lblImg;
    QPushButton *btnTakeAPicture;
    QLabel *lblPicture;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(656, 620);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        lblImg = new QLabel(centralwidget);
        lblImg->setObjectName(QString::fromUtf8("lblImg"));
        lblImg->setGeometry(QRect(5, 5, 640, 480));
        lblImg->setFrameShape(QFrame::Shape::Box);
        lblImg->setFrameShadow(QFrame::Shadow::Raised);
        lblImg->setScaledContents(true);
        btnTakeAPicture = new QPushButton(centralwidget);
        btnTakeAPicture->setObjectName(QString::fromUtf8("btnTakeAPicture"));
        btnTakeAPicture->setGeometry(QRect(250, 500, 161, 51));
        btnTakeAPicture->setStyleSheet(QString::fromUtf8("font: 12pt \"\353\247\221\354\235\200 \352\263\240\353\224\225\";"));
        lblPicture = new QLabel(centralwidget);
        lblPicture->setObjectName(QString::fromUtf8("lblPicture"));
        lblPicture->setGeometry(QRect(10, 490, 101, 81));
        lblPicture->setStyleSheet(QString::fromUtf8("border-color: rgb(145, 92, 93);\n"
"border-style:solid;\n"
"border-width:1px;\n"
""));
        lblPicture->setScaledContents(true);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 656, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        lblImg->setText(QString());
        btnTakeAPicture->setText(QCoreApplication::translate("MainWindow", "Take a picture", nullptr));
        lblPicture->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
