QT += core gui widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = thakur
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/splashscreen.cpp \
    src/script_hub.cpp \
    src/ipc/ipc_client.cpp

HEADERS += \
    src/mainwindow.h \
    src/splashscreen.h \
    src/injector.h \
    src/memory_scanner.h \
    src/process_manager.h \
    src/script_hub.h \
    src/security_guard.h \
    src/thakur_env.h \
    src/unc_suite.h \
    src/ipc/ipc_client.h

FORMS += \
    ui/mainwindow.ui \
    ui/splashscreen.ui

RESOURCES += \
    resources.qrc

LIBS += -luser32 -lshell32
