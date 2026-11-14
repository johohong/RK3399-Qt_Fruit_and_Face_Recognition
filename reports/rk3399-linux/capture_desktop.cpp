#include <QApplication>
#include <QScreen>
#include <QPixmap>
#include <QDebug>
int main(int argc,char** argv){QApplication app(argc,argv);if(argc!=2)return 2;auto screen=app.primaryScreen();if(!screen)return 3;qInfo()<<screen->size();return screen->grabWindow(0).save(QString::fromLocal8Bit(argv[1]))?0:4;}
