#include <QApplication>
#include <QMainWindow>
#include <QTimer>
int main(int argc,char** argv){QApplication app(argc,argv);QMainWindow w;w.setWindowTitle("FruitDetect Qt smoke");w.resize(640,400);w.show();if(app.arguments().contains("--smoke"))QTimer::singleShot(3000,&app,&QApplication::quit);return app.exec();}
