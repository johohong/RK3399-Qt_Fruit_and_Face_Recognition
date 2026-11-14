#!/bin/bash
# ============================================================
# OpenCV 4.8.0 编译脚本 - NanoPi OPI4 (RK3399, Cortex-A72/A53)
# ============================================================
# 修正说明：
#   RK3399 的 Cortex-A72/A53 属于 ARMv8.0-A 架构，
#   不支持 FP16 SIMD 指令（需要 ARMv8.2-A）。
#
#   原始命令中以下三个 FP16 相关选项导致了编译错误：
#     1. -mcpu=...+fp16  → 使 GCC 定义 __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
#                          → 触发 CONV_ARM_FP16 宏 → 编译 FP16 代码路径
#     2. -mfp16-format=ieee → 使 GCC 定义 __ARM_FP16_FORMAT_IEEE（非必要）
#     3. -DCV_FP16=1 → 被 CMake 生成的 cv_cpu_config.h 中的 #define CV_FP16 0 覆盖
#
#   问题链：
#     +fp16 → CONV_ARM_FP16 定义 → convolution.cpp 中 FP16 代码被编译
#     但 CPU_BASELINE/DISPATCH=NEON（不含 FP16）→ CV_FP16=0
#     → FastConv 结构体中 FP16 成员未定义 → 编译报错
#
#   解决方案：移除所有 FP16 相关标志
# ============================================================

cd /home/pi/work/opencv-4.8.0/build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_C_FLAGS="-mcpu=cortex-a72.cortex-a53+crypto+crc -mtune=cortex-a72.cortex-a53" \
    -DCMAKE_CXX_FLAGS="-mcpu=cortex-a72.cortex-a53+crypto+crc -mtune=cortex-a72.cortex-a53" \
	-DENABLE_NEON=ON \
    -DCPU_BASELINE=NEON \
    -DCPU_DISPATCH=NEON \
    -DWITH_OPENMP=ON \
    -DWITH_OPENCL=OFF \
    -DWITH_GSTREAMER=ON \
    -DWITH_FFMPEG=ON \
    -DWITH_V4L=ON \
    -DWITH_GTK=OFF \
    -DWITH_OPENGL=OFF \
    -DOPENCV_ENABLE_NONFREE=OFF \
    -DOPENCV_EXTRA_MODULES_PATH=/home/pi/work/opencv_contrib-4.8.0/modules \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_DOCS=OFF \
    -DBUILD_opencv_python3=OFF \
    -DBUILD_opencv_python2=OFF \
    -DBUILD_opencv_apps=ON \
    -DBUILD_opencv_dnn=ON \
    -DBUILD_opencv_face=OFF \
    -DBUILD_opencv_wechat_qrcode=OFF \
    -DBUILD_opencv_xfeatures2d=OFF \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_SKIP_RPATH=OFF \
    -DCMAKE_INSTALL_RPATH=/usr/local/lib \
    -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
    -DOPENCV_GENERATE_PKGCONFIG=ON

# 编译（根据核心数调整 -j 参数，RK3399 为 6 核）
make -j6

# 安装
# sudo make install
# sudo ldconfig
