$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$pythonExe = Join-Path $repoRoot '.tools\esp-venv\Scripts\python.exe'
$buildRoot = Join-Path $env:USERPROFILE '.cache\ardubot-mic-test'
$coreRoot = Join-Path $env:USERPROFILE '.cache\ardubot-pio'
$tempRoot = Join-Path $env:USERPROFILE '.cache\ardubot-tmp'
$oldCore = $env:PLATFORMIO_CORE_DIR
$oldTemp = $env:TEMP
$oldTmp = $env:TMP
try {
    $env:PLATFORMIO_CORE_DIR = $coreRoot
    $env:TEMP = $tempRoot
    $env:TMP = $tempRoot
    New-Item -ItemType Directory -Force -Path $tempRoot,(Join-Path $buildRoot 'src') | Out-Null
    foreach ($name in @('platformio.ini', 'CMakeLists.txt', 'sdkconfig.defaults')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $buildRoot
    }
    foreach ($name in @('main.c', 'CMakeLists.txt')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot "src\$name") -Destination (Join-Path $buildRoot 'src')
    }
    & $pythonExe -m platformio run -d $buildRoot
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
    $outputDir = Join-Path $repoRoot '.tools\mic-build'
    New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
    $artifactDir = Join-Path $buildRoot '.pio\build\mic-test'
    foreach ($name in @('firmware.bin', 'bootloader.bin', 'partitions.bin', 'firmware.elf')) {
        Copy-Item -LiteralPath (Join-Path $artifactDir $name) -Destination $outputDir
    }
    Write-Output "Build artifacts: $outputDir"
} finally {
    $env:PLATFORMIO_CORE_DIR = $oldCore
    $env:TEMP = $oldTemp
    $env:TMP = $oldTmp
}
