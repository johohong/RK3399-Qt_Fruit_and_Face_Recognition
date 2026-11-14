#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
output="${1:?Absolute output directory required}"
[[ "$output" = /* ]] || { printf '%s\n' 'OUTPUT_MUST_BE_ABSOLUTE' >&2; exit 2; }
for name in models face-models; do
    mkdir -p -- "$output/$name"
    while IFS= read -r -d '' source; do cp -- "$source" "$output/$name/"; done < <(find "$project_root/$name" -maxdepth 1 -type f -print0)
done
printf '%s\n' 'LINUX_MODEL_DEPLOY_PASS: Qt/OpenCV/MNN shared libraries must already be installed on board'
