#include "album.h"
#include "photos.h"
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QImageReader>
#include <QFileInfo>
Album::Album(const QString& directory,QWidget* parent):QDialog(parent),directory_(directory){
 setWindowTitle("相册");resize(1000,700);auto layout=new QVBoxLayout(this);info_=new QLabel;info_->setWordWrap(true);layout->addWidget(info_);
 auto row=new QHBoxLayout;files_=new QListWidget;files_->setMaximumWidth(300);image_=new QLabel("请选择照片");image_->setAlignment(Qt::AlignCenter);image_->setMinimumSize(500,400);row->addWidget(files_);row->addWidget(image_,1);layout->addLayout(row,1);
 auto buttons=new QHBoxLayout;layout->addLayout(buttons);auto exitButton=new QPushButton("退出");exitButton->setObjectName("exitAlbum");buttons->addWidget(exitButton);connect(exitButton,&QPushButton::clicked,this,&QDialog::reject);auto folder=new QPushButton("浏览其他目录");auto reload=new QPushButton("刷新");auto previous=new QPushButton("上一张");auto next=new QPushButton("下一张");auto use=new QPushButton("打开到检测界面");for(auto b:{folder,reload,previous,next,use})buttons->addWidget(b);
 connect(folder,&QPushButton::clicked,this,[this]{auto p=QFileDialog::getExistingDirectory(this,"浏览相册",directory_);if(!p.isEmpty()){directory_=p;refresh();}});
 connect(reload,&QPushButton::clicked,this,&Album::refresh);connect(files_,&QListWidget::currentRowChanged,this,[this]{preview();});
 connect(previous,&QPushButton::clicked,this,[this]{if(files_->currentRow()>0)files_->setCurrentRow(files_->currentRow()-1);});
 connect(next,&QPushButton::clicked,this,[this]{if(files_->currentRow()+1<files_->count())files_->setCurrentRow(files_->currentRow()+1);});
 connect(use,&QPushButton::clicked,this,[this]{if(auto item=files_->currentItem()){emit imageSelected(item->data(Qt::UserRole).toString());accept();}});refresh();
}
void Album::refresh(){files_->clear();for(const auto& path:Photos::list(directory_)){auto item=new QListWidgetItem(QFileInfo(path).fileName(),files_);item->setData(Qt::UserRole,path);item->setToolTip(path);}info_->setText(directory_+QString("\n共 %1 张照片").arg(files_->count()));image_->clear();if(files_->count())files_->setCurrentRow(0);else image_->setText("目录中没有照片");}
void Album::preview(){auto item=files_->currentItem();if(!item)return;QImageReader reader(item->data(Qt::UserRole).toString());reader.setAutoTransform(true);auto size=reader.size();if(size.isValid())reader.setScaledSize(size.scaled(image_->size(),Qt::KeepAspectRatio));QImage im=reader.read();if(im.isNull()){image_->setText("无法读取照片："+reader.errorString());return;}image_->setPixmap(QPixmap::fromImage(im).scaled(image_->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));}
