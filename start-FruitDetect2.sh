#!/usr/bin/env bash
# RK3399 板端启动脚本（通用版）
#
# 相比原始脚本的改动：
#   1. 不再硬编码 /home/pi/work/... 绝对路径，自动定位本脚本同级或上级的构建产物；
#   2. XAUTHORITY 跟随当前 $HOME，不再写死 pi 用户；
#   3. 摄像头节点支持 FRUIT_CAMERA 环境变量覆盖；by-id 路径不存在时自动枚举回退；
#   4. 启动前检查 ldd，出现 not found 直接报错退出，不静默失败。
#
# 用法：
#   bash start-FruitDetect2.sh [传给 FruitDetect2 的额外参数...]
#   FRUIT_CAMERA=/dev/video2 bash start-FruitDetect2.sh
#   FRUIT_THREADS=6 bash start-FruitDetect2.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ---------- 1. 定位可执行文件 ----------
# 依次尝试常见的构建输出目录，也支持用 FRUIT_BIN 直接指定
CANDIDATES=(
    "${FRUIT_BIN:-}"
    "$SCRIPT_DIR/build-agent-release/bin/FruitDetect2"
    "$SCRIPT_DIR/build-linux/bin/FruitDetect2"
    "$SCRIPT_DIR/build/bin/FruitDetect2"
    "$SCRIPT_DIR/FruitDetect2"
)

BIN=""
for c in "${CANDIDATES[@]}"; do
    [ -z "$c" ] && continue
    if [ -x "$c" ]; then BIN="$c"; break; fi
done

if [ -z "$BIN" ]; then
    echo "找不到可执行文件 FruitDetect2。" >&2
    echo "已尝试以下路径：" >&2
    for c in "${CANDIDATES[@]}"; do [ -n "$c" ] && echo "  $c" >&2; done
    echo >&2
    echo "请先构建，或用 FRUIT_BIN=/abs/path/FruitDetect2 指定：" >&2
    echo "  cmake -S \"$SCRIPT_DIR\" -B \"$SCRIPT_DIR/build-linux\" -DCMAKE_BUILD_TYPE=Release \\" >&2
    echo "        -DOpenCV_DIR=/usr/local/lib/cmake/opencv4 -DMNN_ROOT=/usr/local" >&2
    echo "  cmake --build \"$SCRIPT_DIR/build-linux\" --parallel 2" >&2
    exit 1
fi

# ---------- 2. 桌面会话环境 ----------
export DISPLAY="${DISPLAY:-:0}"
# 跟随当前用户的 HOME，不写死 pi
export XAUTHORITY="${XAUTHORITY:-$HOME/.Xauthority}"

if [ ! -e "$XAUTHORITY" ]; then
    echo "提示：XAUTHORITY ($XAUTHORITY) 不存在，若窗口无法显示请检查桌面会话。" >&2
fi

# ---------- 3. 依赖完整性检查 ----------
if command -v ldd >/dev/null 2>&1; then
    MISSING="$(ldd "$BIN" 2>&1 | grep -i 'not found' || true)"
    if [ -n "$MISSING" ]; then
        echo "动态依赖缺失，无法启动：" >&2
        echo "$MISSING" >&2
        echo >&2
        echo "常见原因：板端 MNN/OpenCV 前缀与构建时不一致。" >&2
        echo "检查 /usr/local/include/MNN/Interpreter.hpp 与 /usr/local/lib/libMNN.so 是否存在，" >&2
        echo "或重新用 -DMNN_ROOT=<实际前缀> 配置构建。" >&2
        exit 1
    fi
fi

# ---------- 4. 摄像头节点 ----------
# 优先级：命令行 --camera > FRUIT_CAMERA 环境变量 > 已验证的 by-id 稳定路径 > 自动枚举
VERIFIED_BY_ID="/dev/v4l/by-id/usb-ARKMICRO_USB2.0_PC_CAMERA-video-index0"

find_camera() {
    # 1) 已验证的 by-id 稳定路径（同一台板子上最可靠，不受插拔顺序影响）
    if [ -e "$VERIFIED_BY_ID" ]; then
        echo "$VERIFIED_BY_ID"
        return 0
    fi

    # 2) 若有其他 by-id USB 摄像头节点，取第一个
    if [ -d /dev/v4l/by-id ]; then
        local byid
        byid="$(ls -1 /dev/v4l/by-id/*usb*video*index0 2>/dev/null | head -n 1 || true)"
        if [ -n "$byid" ]; then
            echo "$byid"
            return 0
        fi
    fi

    # 3) 回退：枚举 /dev/video*，挑一个具备 CAPTURE 能力的节点
    local dev
    for dev in /dev/video*; do
        [ -e "$dev" ] || continue
        if command -v v4l2-ctl >/dev/null 2>&1; then
            if v4l2-ctl -d "$dev" --info 2>/dev/null | grep -q 'Video Capture'; then
                echo "$dev"
                return 0
            fi
        else
            # 没有 v4l2-ctl 时只能给第一个节点，需用户自行确认
            echo "$dev"
            return 0
        fi
    done

    return 1
}

ARGS=(--backend mnn --threads "${FRUIT_THREADS:-4}")

# 用户是否已在额外参数里传了 --camera
USER_GAVE_CAMERA=0
for a in "$@"; do
    case "$a" in
        --camera|--camera=*) USER_GAVE_CAMERA=1 ;;
    esac
done

if [ "$USER_GAVE_CAMERA" -eq 0 ]; then
    if [ -n "${FRUIT_CAMERA:-}" ]; then
        CAMERA="$FRUIT_CAMERA"
    else
        CAMERA="$(find_camera || true)"
    fi

    if [ -n "$CAMERA" ]; then
        if [ -e "$CAMERA" ]; then
            ARGS+=(--camera "$CAMERA")
        else
            echo "警告：摄像头节点 $CAMERA 不存在，将不传 --camera（程序启动后需手动打开）。" >&2
        fi
    else
        echo "警告：未找到任何 /dev/video* 节点，将不传 --camera。" >&2
        echo "      可用 v4l2-ctl --list-devices 确认实际采集节点。" >&2
    fi
fi

# ---------- 5. 启动 ----------
echo "程序      : $BIN"
echo "DISPLAY   : $DISPLAY"
echo "XAUTHORITY: $XAUTHORITY"
if [ "${#ARGS[@]}" -gt 2 ]; then
    echo "摄像头    : ${ARGS[-1]}"
fi
echo "参数      : ${ARGS[*]} $*"
echo
echo "提示：程序启动后不会自动打开摄像头，需在界面点击「打开摄像头」。管理员初始密码 123。"
echo

exec "$BIN" "${ARGS[@]}" "$@"
