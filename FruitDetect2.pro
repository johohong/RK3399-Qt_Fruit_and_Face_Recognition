TEMPLATE = app
TARGET = FruitDetect2
QT += core gui widgets concurrent
CONFIG += c++17 console
CONFIG -= debug debug_and_release
CONFIG += release
INCLUDEPATH += $$PWD/src
DEFINES += FRUIT_WITH_MNN
SOURCES += $$PWD/src/main.cpp $$PWD/src/mainwindow.cpp $$PWD/src/worker.cpp \
    $$PWD/src/detector.cpp $$PWD/src/postprocess.cpp $$PWD/src/io.cpp \
    $$PWD/src/photos.cpp $$PWD/src/album.cpp $$PWD/src/face_engine.cpp $$PWD/src/face_dialog.cpp
HEADERS += $$files($$PWD/src/*.h)
DESTDIR = $$OUT_PWD/bin
win32 {
    QMAKE_CXXFLAGS += /utf-8
    DEFINES += USING_MNN_DLL
    isEmpty(OPENCV_ROOT): OPENCV_ROOT = E:/Tool/opencv-4.8.0-windows/opencv/build
    isEmpty(MNN_ROOT): MNN_ROOT = $$PWD/third_party/mnn-windows
    INCLUDEPATH += $$OPENCV_ROOT/include $$MNN_ROOT/include
    LIBS += -L$$OPENCV_ROOT/x64/vc16/lib -lopencv_world480
    LIBS += -L$$MNN_ROOT/lib -lMNN
    QMAKE_POST_LINK += powershell.exe -NoProfile -ExecutionPolicy Bypass -File $$shell_quote($$shell_path($$PWD/scripts/deploy_windows.ps1)) -OutputDirectory $$shell_quote($$shell_path($$DESTDIR))
}
unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4
    isEmpty(MNN_ROOT): MNN_ROOT = /usr/local
    INCLUDEPATH += $$MNN_ROOT/include
    LIBS += -L$$MNN_ROOT/lib -lMNN -lpthread
    QMAKE_RPATHDIR += $$MNN_ROOT/lib
    QMAKE_POST_LINK += bash $$shell_quote($$PWD/scripts/deploy_linux.sh) $$shell_quote($$DESTDIR)
}
