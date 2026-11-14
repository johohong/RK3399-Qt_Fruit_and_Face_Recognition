param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$project=Split-Path -Parent $PSScriptRoot
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
foreach($name in @('models','face-models')){New-Item -ItemType Directory -Path (Join-Path $OutputDirectory $name) -Force | Out-Null;Get-ChildItem -LiteralPath (Join-Path $project $name) -File | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $OutputDirectory $name) -Force}}
Copy-Item -LiteralPath "$project\third_party\mnn-windows\bin\MNN.dll" -Destination $OutputDirectory -Force
Copy-Item -LiteralPath 'E:\Tool\opencv-4.8.0-windows\opencv\build\x64\vc16\bin\opencv_world480.dll' -Destination $OutputDirectory -Force
Copy-Item -LiteralPath 'E:\Tool\opencv-4.8.0-windows\opencv\build\x64\vc16\bin\opencv_videoio_ffmpeg480_64.dll' -Destination $OutputDirectory -Force
$env:VCINSTALLDIR='E:\Tool\Visual Studio 2017\2017\Community\VC\'
& 'E:\Tool\Qt\Qt5.12.12\5.12.12\msvc2017_64\bin\windeployqt.exe' --release --compiler-runtime --no-translations (Join-Path $OutputDirectory 'FruitDetect2.exe')
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
Write-Output 'E_DRIVE_DEPLOY_PASS'
exit 0
