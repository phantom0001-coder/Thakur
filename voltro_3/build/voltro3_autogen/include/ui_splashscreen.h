/********************************************************************************
** Form generated from reading UI file 'splashscreen.ui'
**
** Created by: Qt User Interface Compiler version 6.4.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SPLASHSCREEN_H
#define UI_SPLASHSCREEN_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SplashScreen
{
public:
    QVBoxLayout *splashLayout;
    QLabel *lblLogoImage;
    QLabel *lblTitle;
    QLabel *lblSub;
    QSpacerItem *verticalSpacer;
    QProgressBar *progressBar;
    QLabel *lblStatus;

    void setupUi(QWidget *SplashScreen)
    {
        if (SplashScreen->objectName().isEmpty())
            SplashScreen->setObjectName("SplashScreen");
        SplashScreen->resize(500, 340);
        SplashScreen->setStyleSheet(QString::fromUtf8("\n"
"    QWidget#SplashScreen { background-color: #07090c; border: 1px solid #1e2836; border-radius: 12px; color: #e8eef4; }\n"
"    QLabel#lblTitle { font-family: 'Orbitron'; font-size: 26px; font-weight: 900; color: #00d2ff; letter-spacing: 2px; }\n"
"    QLabel#lblSub { font-family: 'Inter'; font-size: 11px; color: #8b9bb0; letter-spacing: 3px; font-weight: 600; }\n"
"    QProgressBar { border: 1px solid #1e2836; border-radius: 4px; background: #0c1016; text-align: center; height: 6px; }\n"
"    QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00d2ff, stop:0.5 #0072ff, stop:1 #8000ff); border-radius: 3px; }\n"
"   "));
        splashLayout = new QVBoxLayout(SplashScreen);
        splashLayout->setSpacing(12);
        splashLayout->setObjectName("splashLayout");
        splashLayout->setContentsMargins(40, 36, 40, 36);
        lblLogoImage = new QLabel(SplashScreen);
        lblLogoImage->setObjectName("lblLogoImage");
        lblLogoImage->setMinimumSize(QSize(110, 110));
        lblLogoImage->setMaximumSize(QSize(110, 110));
        lblLogoImage->setPixmap(QPixmap(QString::fromUtf8(":/resources/images/voltro_logo.png")));
        lblLogoImage->setScaledContents(true);
        lblLogoImage->setAlignment(Qt::AlignCenter);

        splashLayout->addWidget(lblLogoImage);

        lblTitle = new QLabel(SplashScreen);
        lblTitle->setObjectName("lblTitle");
        lblTitle->setAlignment(Qt::AlignCenter);

        splashLayout->addWidget(lblTitle);

        lblSub = new QLabel(SplashScreen);
        lblSub->setObjectName("lblSub");
        lblSub->setAlignment(Qt::AlignCenter);

        splashLayout->addWidget(lblSub);

        verticalSpacer = new QSpacerItem(20, 15, QSizePolicy::Minimum, QSizePolicy::Expanding);

        splashLayout->addItem(verticalSpacer);

        progressBar = new QProgressBar(SplashScreen);
        progressBar->setObjectName("progressBar");
        progressBar->setValue(0);
        progressBar->setTextVisible(false);

        splashLayout->addWidget(progressBar);

        lblStatus = new QLabel(SplashScreen);
        lblStatus->setObjectName("lblStatus");
        lblStatus->setAlignment(Qt::AlignCenter);
        lblStatus->setStyleSheet(QString::fromUtf8("color: #00d2ff; font-size: 11px; font-family: 'JetBrains Mono';"));

        splashLayout->addWidget(lblStatus);


        retranslateUi(SplashScreen);

        QMetaObject::connectSlotsByName(SplashScreen);
    } // setupUi

    void retranslateUi(QWidget *SplashScreen)
    {
        SplashScreen->setWindowTitle(QCoreApplication::translate("SplashScreen", "VOLTRO Loading...", nullptr));
        lblLogoImage->setText(QString());
        lblTitle->setText(QCoreApplication::translate("SplashScreen", "VOLTRO", nullptr));
        lblSub->setText(QCoreApplication::translate("SplashScreen", "VOLTRO LUAU EXECUTOR ENGINE", nullptr));
        lblStatus->setText(QCoreApplication::translate("SplashScreen", "Initializing Voltro Runtime Environment...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SplashScreen: public Ui_SplashScreen {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPLASHSCREEN_H
