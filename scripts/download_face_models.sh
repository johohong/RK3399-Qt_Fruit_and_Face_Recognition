#!/usr/bin/env bash
# 按 face-models/sources.json 下载 YuNet / SFace 人脸模型并校验 SHA256。
#
# face_recognition_sface_2021dec.onnx 约 36.9 MB，不纳入 Git 仓库。
# 校验不通过即删除半成品并返回非零退出码。
#
# 用法:
#   bash scripts/download_face_models.sh [--force] [sources.json 路径]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

FORCE=0
SOURCES_PATH=""

while [ $# -gt 0 ]; do
    case "$1" in
        --force) FORCE=1; shift ;;
        -h|--help)
            sed -n '2,10p' "${BASH_SOURCE[0]}"
            exit 0
            ;;
        *) SOURCES_PATH="$1"; shift ;;
    esac
done

if [ -z "$SOURCES_PATH" ]; then
    SOURCES_PATH="$REPO_ROOT/face-models/sources.json"
fi

if [ ! -f "$SOURCES_PATH" ]; then
    echo "找不到来源清单: $SOURCES_PATH" >&2
    exit 1
fi

TARGET_DIR="$(dirname "$SOURCES_PATH")"
echo "来源清单: $SOURCES_PATH"
echo "目标目录: $TARGET_DIR"
echo

# 选择下载工具
if command -v curl >/dev/null 2>&1; then
    FETCH="curl"
elif command -v wget >/dev/null 2>&1; then
    FETCH="wget"
else
    echo "需要 curl 或 wget，请先安装。" >&2
    exit 1
fi

# 选择哈希工具
if command -v sha256sum >/dev/null 2>&1; then
    hash_of() { sha256sum "$1" | awk '{print $1}'; }
elif command -v shasum >/dev/null 2>&1; then
    hash_of() { shasum -a 256 "$1" | awk '{print $1}'; }
else
    echo "需要 sha256sum 或 shasum，请先安装。" >&2
    exit 1
fi

size_of() {
    if stat -c%s "$1" >/dev/null 2>&1; then
        stat -c%s "$1"
    else
        stat -f%z "$1"
    fi
}

# 解析 sources.json：优先用 python3，无则用 grep/sed 兜底
parse_entries() {
    if command -v python3 >/dev/null 2>&1; then
        python3 - "$SOURCES_PATH" <<'PY'
import json, sys
with open(sys.argv[1], encoding='utf-8') as f:
    for e in json.load(f):
        print(f"{e['name']}\t{e['sha256']}\t{e['bytes']}\t{e['source']}")
PY
    else
        python - "$SOURCES_PATH" <<'PY'
import json, sys
with open(sys.argv[1], encoding='utf-8') as f:
    for e in json.load(f):
        print(f"{e['name']}\t{e['sha256']}\t{e['bytes']}\t{e['source']}")
PY
    fi
}

FAILED=()
TOTAL=0

while IFS=$'\t' read -r NAME EXPECT EXPECT_BYTES URL; do
    [ -z "$NAME" ] && continue
    TOTAL=$((TOTAL + 1))
    TARGET="$TARGET_DIR/$NAME"
    TEMP="$TARGET.downloading"

    if [ -f "$TARGET" ] && [ "$FORCE" -eq 0 ]; then
        ACTUAL="$(hash_of "$TARGET" | tr 'a-f' 'A-F')"
        if [ "$ACTUAL" = "$(echo "$EXPECT" | tr 'a-f' 'A-F')" ]; then
            echo "[跳过] $NAME 已存在且校验通过"
            continue
        fi
        echo "[重下] $NAME 已存在但校验不匹配"
    fi

    echo "[下载] $NAME"
    echo "       $URL"

    rm -f "$TEMP"
    if [ "$FETCH" = "curl" ]; then
        curl -fSL --retry 2 -o "$TEMP" "$URL"
    else
        wget -O "$TEMP" "$URL"
    fi

    ACTUAL_BYTES="$(size_of "$TEMP")"
    if [ "$ACTUAL_BYTES" != "$EXPECT_BYTES" ]; then
        echo "[失败] $NAME : 字节数不匹配 (期望 $EXPECT_BYTES, 实际 $ACTUAL_BYTES)"
        rm -f "$TEMP"
        FAILED+=("$NAME")
        echo
        continue
    fi

    ACTUAL="$(hash_of "$TEMP" | tr 'a-f' 'A-F')"
    if [ "$ACTUAL" != "$(echo "$EXPECT" | tr 'a-f' 'A-F')" ]; then
        echo "[失败] $NAME : SHA256 不匹配"
        rm -f "$TEMP"
        FAILED+=("$NAME")
        echo
        continue
    fi

    mv -f "$TEMP" "$TARGET"
    MB=$((ACTUAL_BYTES / 1024 / 1024))
    echo "[完成] $NAME (${MB} MB, SHA256 校验通过)"
    echo
done < <(parse_entries)

if [ ${#FAILED[@]} -gt 0 ]; then
    echo "以下模型下载或校验失败: ${FAILED[*]}" >&2
    echo "请检查网络，或按 README 第六节的地址手动下载后放入 face-models/ 目录。" >&2
    exit 1
fi

echo "全部模型就绪，共 $TOTAL 个。"
exit 0
