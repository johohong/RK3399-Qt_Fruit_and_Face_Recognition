#include "mainwindow.h"
#include "io.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QDebug>
int main(int argc,char** argv){QApplication app(argc,argv);QCommandLineParser parser;addDetectorArguments(parser);
#ifdef _WIN32
const QString cameraDefault="0";
#else
const QString cameraDefault="/dev/video0";
#endif
parser.addOption({"camera","Camera device","device",cameraDefault});parser.addOption({"smoke-image","Automatically detect image and exit","path"});parser.addOption({"smoke-camera","Open camera and perform detection for seconds","seconds"});parser.addOption({"camera-cycles","Open/close cycles before continuous camera smoke","count","0"});if(!parser.parse(app.arguments())){qCritical()<<parser.errorText();return 2;}if(parser.isSet("help"))parser.showHelp();DetectorOptions options;QString error;if(!parseDetectorArguments(parser,options,error)){qCritical()<<error;return 2;}MainWindow window(options,parser.value("camera"));window.show();
if(parser.isSet("smoke-image")){auto poll=new QTimer(&app);poll->setInterval(100);auto started=std::make_shared<bool>(false);QObject::connect(poll,&QTimer::timeout,&app,[&window,&app,&parser,started]{if(!window.failure().isEmpty()){qCritical()<<window.failure();app.exit(3);return;}if(window.ready()&&!*started){*started=true;if(!window.openImage(parser.value("smoke-image"))){app.exit(4);return;}window.detectImage();}if(window.completedCount()>0){qInfo()<<"GUI_IMAGE_PASS";window.close();app.exit(0);}});poll->start();QTimer::singleShot(60000,&app,[&app]{app.exit(5);});}
if(parser.isSet("smoke-camera")){int seconds=parser.value("smoke-camera").toInt();int cycles=parser.value("camera-cycles").toInt();auto tick=new QTimer(&app);tick->setInterval(1000);auto state=std::make_shared<int>(0);QObject::connect(tick,&QTimer::timeout,&app,[&window,&app,tick,state,seconds,cycles]{if(!window.failure().isEmpty()){qCritical()<<window.failure();app.exit(4);return;}if(!window.ready())return;if(*state<cycles*2){if(*state%2==0){if(!window.openCamera()){app.exit(4);return;}}else window.closeCamera();++*state;return;}if(*state==cycles*2&&!window.openCamera()){app.exit(4);return;}window.detectImage();++*state;if(*state>=cycles*2+seconds){qInfo()<<"GUI_CAMERA_COMPLETED"<<window.completedCount();window.close();app.exit(window.completedCount()>0?0:5);}});tick->start();QTimer::singleShot((seconds+cycles*2+60)*1000,&app,[&app]{app.exit(5);});}
return app.exec();}
