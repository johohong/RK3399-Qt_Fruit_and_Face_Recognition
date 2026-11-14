<#
.SYNOPSIS
    按 face-models/sources.json 下载 YuNet / SFace 人脸模型并校验 SHA256。

.DESCRIPTION
    face_recognition_sface_2021dec.onnx 约 36.9 MB，不纳入 Git 仓库。
    本脚本从 OpenCV Zoo 官方地址下载，逐个校验 SHA256；不匹配即报错退出，
    并删除下载失败的文件，不会留下半个损坏文件。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts/download_face_models.ps1
#>

[CmdletBinding()]
param(
    [string]$SourcesPath,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

# 定位仓库根目录（脚本位于 scripts/ 下）
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $SourcesPath) { $SourcesPath = Join-Path $repoRoot 'face-models\sources.json' }
$targetDir = Split-Path -Parent $SourcesPath

if (-not (Test-Path -LiteralPath $SourcesPath)) {
    throw "找不到来源清单: $SourcesPath"
}

Write-Host "来源清单: $SourcesPath"
Write-Host "目标目录: $targetDir"
Write-Host ''

$entries = Get-Content -LiteralPath $SourcesPath -Raw -Encoding UTF8 | ConvertFrom-Json
$failed = @()

foreach ($entry in $entries) {
    $name       = $entry.name
    $expected   = $entry.sha256
    $expectedBytes = [long]$entry.bytes
    $url        = $entry.source
    $target     = Join-Path $targetDir $name

    if ((Test-Path -LiteralPath $target) -and -not $Force) {
        $actual = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
        if ($actual -eq $expected) {
            Write-Host "[跳过] $name 已存在且校验通过" -ForegroundColor Green
            continue
        }
        Write-Host "[重下] $name 已存在但校验不匹配" -ForegroundColor Yellow
    }

    Write-Host "[下载] $name"
    Write-Host "       $url"

    $temp = "$target.downloading"
    try {
        if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force }

        # TLS 1.2，兼容较旧的 PowerShell 5.1
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $url -OutFile $temp -UseBasicParsing

        $actualBytes = (Get-Item -LiteralPath $temp).Length
        if ($actualBytes -ne $expectedBytes) {
            throw "字节数不匹配: 期望 $expectedBytes, 实际 $actualBytes"
        }

        $actual = (Get-FileHash -LiteralPath $temp -Algorithm SHA256).Hash
        if ($actual -ne $expected) {
            throw "SHA256 不匹配: 期望 $expected, 实际 $actual"
        }

        Move-Item -LiteralPath $temp -Destination $target -Force
        Write-Host "[完成] $name ($([math]::Round($actualBytes/1MB,2)) MB, SHA256 校验通过)" -ForegroundColor Green
    }
    catch {
        Write-Host "[失败] $name : $($_.Exception.Message)" -ForegroundColor Red
        if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force }
        $failed += $name
    }
    Write-Host ''
}

if ($failed.Count -gt 0) {
    Write-Host "以下模型下载或校验失败: $($failed -join ', ')" -ForegroundColor Red
    Write-Host '请检查网络，或按 README 第六节的地址手动下载后放入 face-models/ 目录。' -ForegroundColor Red
    exit 1
}

Write-Host "全部模型就绪，共 $($entries.Count) 个。" -ForegroundColor Green
exit 0
