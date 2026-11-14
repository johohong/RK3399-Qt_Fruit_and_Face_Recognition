#!/usr/bin/env bash
# Linux / RK3399 板端一键构建脚本。
#
# 补齐 README 中引用但原工程未提供的构建入口。
# 默认复用板端已安装好的 OpenCV 与 MNN，不重装、不覆盖 /usr/local。
#
# 用法:
#   bash scripts/build_linux.sh                    # 默认 Release + 板端调优
#   bash scripts/build_linux.sh --tests            # 额外构建离线测试
#   bash scripts/build_linux.sh --no-tuning        # 关闭 RK3399 -mcpu/-mtune 调优
#   bash scripts/build_linux.sh --opencv-dir /path/to/cmake/opencv4
#   bash scripts/build_linux.sh --mnn-root /usr/local
#   bash scripts/build_linux.sh --build-dir build-linux --jobs 2
#   bash scripts/build_linux.sh --help

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

# ---------- 默认参数（与 docs/rk3399-runbook.md 实测配置一致） ----------
BUILD_DIR="$REPO_ROOT/build-linux"
OPENCV_DIR="/usr/local/lib/cmake/opencv4"
MNN_ROOT="/usr/local"
CXX_COMPILER="/usr/bin/g++"
JOBS=2
BUILD_TESTS=0
RK3399_TUNING=1
BUILD_TYPE="Release"

usage() { sed -n '2,16p' "${BASH_SOURCE[0]}"; exit 0; }

while [ $# -gt 0 ]; do
    case "$1" in
        --tests)        BUILD_TESTS=1; shift ;;
        --no-tuning)    RK3399_TUNING=0; shift ;;
        --tuning)       RK3399_TUNING=1; shift ;;
        --opencv-dir)   OPENCV_DIR="${2:?--opencv-dir 需要参数}"; shift 2 ;;
        --mnn-root)     MNN_ROOT="${2:?--mnn-root 需要参数}"; shift 2 ;;
        --build-dir)    BUILD_DIR="${2:?--build-dir 需要参数}"; shift 2 ;;
        --compiler)     CXX_COMPILER="${2:?--compiler 需要参数}"; shift 2 ;;
        --jobs)         JOBS="${2:?--jobs 需要参数}"; shift 2 ;;
        --debug)        BUILD_TYPE="Debug"; shift ;;
        -h|--help)      usage ;;
        *) echo "未知参数: $1" >&2; exit 2 ;;
    esac
done

echo "仓库根目录 : $REPO_ROOT"
echo "构建目录   : $BUILD_DIR"
echo "OpenCV_DIR : $OPENCV_DIR"
echo "MNN_ROOT   : $MNN_ROOT"
echo "编译器     : $CXX_COMPILER"
echo "构建类型   : $BUILD_TYPE"
echo "并行度     : $JOBS"
echo "离线测试   : $([ "$BUILD_TESTS" -eq 1 ] && echo ON || echo OFF)"
echo "RK3399调优 : $([ "$RK3399_TUNING" -eq 1 ] && echo ON || echo OFF)"
echo

# ---------- 前置检查 ----------
command -v cmake >/dev/null 2>&1 || { echo "找不到 cmake（板端实测 3.18.4）。" >&2; exit 1; }
command -v g++   >/dev/null 2>&1 || { echo "找不到 g++（板端实测 GCC 10.2.1）。" >&2; exit 1; }

ARCH="$(uname -m)"
echo "架构: $ARCH"

# OpenCV 配置目录
if [ ! -f "$OPENCV_DIR/OpenCVConfig.cmake" ]; then
    echo "警告: 未找到 $OPENCV_DIR/OpenCVConfig.cmake" >&2
    echo "      用 --opencv-dir 指定实际路径，或运行 bash scripts/probe_linux.sh 体检。" >&2
fi

# MNN 平台匹配检查：板端必须是 ARM64 的 libMNN.so，不能用 Windows 的 .dll/.lib
if [ ! -f "$MNN_ROOT/lib/libMNN.so" ]; then
    echo "警告: 未找到 $MNN_ROOT/lib/libMNN.so" >&2
    echo "      CMake 会在配置阶段 FATAL_ERROR，不会静默降级到 Windows 库。" >&2
    echo "      板端实测 MNN 版本为 3.6.2。" >&2
fi

# RK3399 调优仅在 AArch64 下开启
if [ "$RK3399_TUNING" -eq 1 ] && [ "$ARCH" != "aarch64" ]; then
    echo "注意: 当前架构为 $ARCH，非 AArch64，自动关闭 FRUIT_RK3399_TUNING。" >&2
    echo "      （CMake 自身也会校验架构，不满足即明确失败，不会偷偷换参数。）" >&2
    RK3399_TUNING=0
fi

# 人脸模型检查
if [ ! -f "$REPO_ROOT/face-models/face_recognition_sface_2021dec.onnx" ]; then
    echo "警告: 人脸模型尚未下载，构建可继续但人脸功能无法运行。" >&2
    echo "      请执行: bash scripts/download_face_models.sh" >&2
fi

# ---------- 配置 ----------
CMAKE_ARGS=(
    -S "$REPO_ROOT"
    -B "$BUILD_DIR"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_CXX_COMPILER="$CXX_COMPILER"
    -DOpenCV_DIR="$OPENCV_DIR"
    -DMNN_ROOT="$MNN_ROOT"
    -DFRUIT_BUILD_TESTS="$([ "$BUILD_TESTS" -eq 1 ] && echo ON || echo OFF)"
    -DFRUIT_RK3399_TUNING="$([ "$RK3399_TUNING" -eq 1 ] && echo ON || echo OFF)"
)

echo "[1/4] CMake 配置..."
cmake "${CMAKE_ARGS[@]}"

# ---------- 编译 ----------
echo
echo "[2/4] 编译 $BUILD_TYPE（并行 $JOBS）..."
cmake --build "$BUILD_DIR" --parallel "$JOBS"

# ---------- 依赖完整性 ----------
echo
echo "[3/4] 检查动态依赖..."
BIN="$BUILD_DIR/bin/FruitDetect2"
if [ -x "$BIN" ]; then
    if ldd "$BIN" | grep -i 'not found'; then
        echo "存在缺失的动态库，见上方 not found 条目。" >&2
        echo "常见原因：MNN/OpenCV 前缀与构建时不一致。" >&2
        exit 1
    fi
    echo "ldd 检查通过，无 not found。"
else
    echo "警告: 未生成 $BIN" >&2
fi

# ---------- 测试 ----------
echo
if [ "$BUILD_TESTS" -eq 1 ]; then
    echo "[4/4] 运行离线测试 (ctest)..."
    ( cd "$BUILD_DIR" && ctest --output-on-failure )
else
    echo "[4/4] 跳过测试（未指定 --tests）"
fi

echo
echo "构建完成。产物目录: $BUILD_DIR/bin"
[ -x "$BIN" ] && ls -lh "$BUILD_DIR/bin" | sed 's/^/  /'

cat <<'EOF'

下一步：
  1. 环境体检   : bash scripts/probe_linux.sh
  2. 启动 GUI    : bash start-FruitDetect2.sh
  3. 无界面验收  : ./build-linux/bin/FruitDetect2Cli --backend mnn --threads 4 \
                     --image /abs/tests/fixtures/apple_12.jpg \
                     --output-json /abs/check.json
  4. 验收顺序建议：先用已知水果图片验证检测 → 再用已知照片登记比对人脸 → 最后测真实摄像头。

注意：程序启动后不会自动打开摄像头，需在界面点击「打开摄像头」；管理员初始密码 123。
      板端摄像头请用 --camera 传绝对设备路径，不要用 /dev/video0（那是板载 ISP 节点）。
EOF
exit 0
