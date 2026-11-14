#pragma once
#include <QDialog>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QUuid>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCoreApplication>
class AdminStore {
 QString path_;
 static QByteArray digest(const QString& password,const QByteArray& salt){QByteArray data=salt+password.toUtf8();for(int i=0;i<100000;++i)data=QCryptographicHash::hash(data+salt,QCryptographicHash::Sha256);return data.toHex();}
public:
 explicit AdminStore(QString path):path_(path){}
 static bool valid(const QString& password){return password.size()>=3&&password.size()<=9;}
 bool verify(const QString& password,QString& error) const {
  error.clear();QFile file(path_);if(!file.exists())return password==QStringLiteral("123");
  if(!file.open(QIODevice::ReadOnly)){error="无法读取管理员密码文件";return false;}
  QJsonParseError parse;auto doc=QJsonDocument::fromJson(file.readAll(),&parse);auto o=doc.object();auto salt=o["salt"].toString().toLatin1(),hash=o["hash"].toString().toLatin1();
  if(parse.error!=QJsonParseError::NoError||o["version"].toInt()!=1||salt.size()!=32||hash.size()!=64){error="管理员密码文件损坏，禁止登录";return false;}
  return digest(password,salt)==hash;
 }
 bool change(const QString& old,const QString& password,QString& error){
  if(!verify(old,error)){if(error.isEmpty())error="当前密码错误";return false;}
  if(!valid(password)){error="请输入3位到9位的新密码";return false;}
  QByteArray salt=QUuid::createUuid().toRfc4122().toHex();auto bytes=QJsonDocument(QJsonObject{{"version",1},{"salt",QString::fromLatin1(salt)},{"hash",QString::fromLatin1(digest(password,salt))}}).toJson();
  QSaveFile file(path_);if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){error="密码保存失败，原密码保持有效";return false;}return true;
 }
};
class AdminDialog:public QDialog {
 AdminStore store_;QLabel* prompt_;QLabel* status_;QLineEdit* input_;QPushButton* change_;QPushButton* submit_;QPushButton* back_;int stage_=0;QString old_,next_;
 void loginPage(){stage_=0;old_.clear();next_.clear();input_->clear();change_->show();change_->setChecked(false);prompt_->setText("请输入管理员密码");submit_->setText("登录");back_->hide();}
 void advance(){QString value=input_->text(),error;
  if(stage_==0){if(!store_.verify(value,error)){status_->setText(error.isEmpty()?"密码错误，请重新输入":error);input_->clear();return;}if(!change_->isChecked()){accept();return;}old_=value;stage_=1;change_->hide();back_->show();prompt_->setText("请输入3位到9位的新密码");submit_->setText("下一步");input_->clear();status_->clear();return;}
  if(stage_==1){if(!AdminStore::valid(value)){status_->setText("新密码长度必须为3位到9位");return;}next_=value;stage_=2;prompt_->setText("请再次输入新密码");submit_->setText("确认修改");input_->clear();status_->clear();return;}
  if(value!=next_){status_->setText("两次密码不一致，请重新输入确认密码，或返回重填");input_->clear();return;}
  if(!store_.change(old_,next_,error)){status_->setText(error);return;}loginPage();status_->setText("修改成功，请使用新密码登录");
 }
public:
 explicit AdminDialog(QWidget* parent=nullptr,QString path=QCoreApplication::applicationDirPath()+"/admin-credentials.json"):QDialog(parent),store_(path){
  setWindowTitle("管理员模式");setMinimumWidth(420);auto layout=new QVBoxLayout(this);prompt_=new QLabel;layout->addWidget(prompt_);input_=new QLineEdit;input_->setObjectName("adminPassword");input_->setEchoMode(QLineEdit::Password);layout->addWidget(input_);
  change_=new QPushButton("更改密码");change_->setObjectName("changePassword");change_->setCheckable(true);change_->setStyleSheet("QPushButton:checked {background-color:#237b45;color:white;border:2px solid #15552d;padding:6px;}");layout->addWidget(change_);
  status_=new QLabel;status_->setObjectName("adminStatus");status_->setWordWrap(true);layout->addWidget(status_);auto row=new QHBoxLayout;layout->addLayout(row);submit_=new QPushButton;submit_->setObjectName("adminSubmit");submit_->setDefault(true);back_=new QPushButton("返回");back_->setObjectName("adminBack");auto exit=new QPushButton("退出");exit->setObjectName("adminExit");for(auto b:{submit_,back_,exit})row->addWidget(b);
  connect(change_,&QPushButton::toggled,this,[this](bool checked){prompt_->setText(checked?"修改密码：请输入当前管理员密码":"请输入管理员密码");submit_->setText(checked?"验证当前密码":"登录");status_->setText(checked?"已选择更改密码":"");});
  connect(submit_,&QPushButton::clicked,this,[this]{advance();});connect(back_,&QPushButton::clicked,this,[this]{if(stage_==2){stage_=1;next_.clear();input_->clear();prompt_->setText("请输入3位到9位的新密码");submit_->setText("下一步");status_->clear();}else{loginPage();status_->clear();}});connect(exit,&QPushButton::clicked,this,&QDialog::reject);loginPage();
 }
};
