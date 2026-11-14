#!/bin/sh
sudo apt update
sudo apt install -y language-pack-zh-hans locales fonts-wqy-zenhei
sudo locale-gen zh_CN.UTF-8
sudo update-locale LANG=zh_CN.UTF-8 LC_ALL=zh_CN.UTF-8
echo "set encoding=utf-8
set termencoding=utf-8
set fileencoding=utf-8
set fileencodings=utf-8,gb18030,gbk,gb2312,latin1" > ~/.vimrc
export LANG=zh_CN.UTF-8
export LC_ALL=zh_CN.UTF-8
echo "修复完成，重新打开vi测试中文"
