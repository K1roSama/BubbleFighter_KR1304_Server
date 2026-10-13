[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [string]$MySqlRoot,
    [ValidateRange(1, 16)]
    [int]$ParallelBuildCount = 4
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($MySqlRoot)) {
    $MySqlRoot = Join-Path $repositoryRoot 'Artifacts/Dependencies/mysql-8.4.11-winx64'
}
$MySqlRoot = (Resolve-Path -LiteralPath $MySqlRoot).Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio Installer/vswhere.exe is required.'
}
$installationPath = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installationPath)) {
    throw 'A Visual Studio installation with MSBuild and MSVC is required.'
}
$msbuild = Join-Path $installationPath 'MSBuild/Current/Bin/MSBuild.exe'
$logs = Join-Path $repositoryRoot 'Artifacts/Logs'
New-Item -ItemType Directory -Force -Path $logs | Out-Null
$buildLog = Join-Path $logs "build-$Configuration.log"
$errorLog = Join-Path $logs "build-$Configuration.errors.log"

Push-Location $repositoryRoot
try {
    & $msbuild BubbleFighter.Server.sln /m:2 /nologo /v:minimal `
        "/p:Configuration=$Configuration" /p:Platform=x64 `
        "/p:CL_MPCount=$ParallelBuildCount" "/p:MySqlRoot=$MySqlRoot" `
        "/flp:logfile=$buildLog;verbosity=normal;encoding=UTF-8" `
        "/flp2:logfile=$errorLog;errorsonly;encoding=UTF-8"
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed for $Configuration; see $buildLog."
    }
}
finally {
    Pop-Location
}
