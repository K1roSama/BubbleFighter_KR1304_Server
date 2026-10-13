[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$Version,
    [string]$MySqlRoot,
    [string]$RuntimeDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^[0-9A-Za-z][0-9A-Za-z._+-]*$') {
    throw 'Version must start with a letter or number and contain only letters, numbers, dot, underscore, plus or hyphen.'
}
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $repositoryRoot 'Artifacts/Bin/x64/Release'
$distributionDirectory = Join-Path $repositoryRoot 'Artifacts/Distribution'
if ([string]::IsNullOrWhiteSpace($MySqlRoot)) {
    $MySqlRoot = Join-Path $repositoryRoot 'Artifacts/Dependencies/mysql-8.4.11-winx64'
}
$MySqlRoot = (Resolve-Path -LiteralPath $MySqlRoot).Path
if ([string]::IsNullOrWhiteSpace($RuntimeDirectory)) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $installationPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installationPath)) {
        throw 'Visual Studio with the x64 MSVC redistributable files is required.'
    }
    $redistRoot = Join-Path $installationPath 'VC/Redist/MSVC'
    $candidates = @(Get-ChildItem -LiteralPath $redistRoot -Directory |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
        Sort-Object { [version]$_.Name } -Descending |
        ForEach-Object { Join-Path $_.FullName 'x64/Microsoft.VC143.CRT' } |
        Where-Object { Test-Path -LiteralPath $_ })
    if ($candidates.Count -eq 0) {
        throw 'The x64 Microsoft.VC143.CRT redistributable directory was not found.'
    }
    $RuntimeDirectory = $candidates[0]
}
$RuntimeDirectory = (Resolve-Path -LiteralPath $RuntimeDirectory).Path
$runtimeFiles = @(Get-ChildItem -LiteralPath $RuntimeDirectory -Filter '*.dll' -File)
foreach ($required in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $RuntimeDirectory $required))) {
        throw "MSVC runtime file is missing: $required"
    }
}
$programFiles = @('BubbleFighter.Server.exe', 'libmysql.dll', 'libssl-3-x64.dll', 'libcrypto-3-x64.dll')
foreach ($file in $programFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $outputDirectory $file))) {
        throw "Build Release before packaging; missing $file."
    }
}
foreach ($file in @('LICENSE', 'README', 'docs/INFO_BIN', 'docs/INFO_SRC')) {
    if (-not (Test-Path -LiteralPath (Join-Path $MySqlRoot $file))) {
        throw "MySQL distribution notice is missing: $file"
    }
}

New-Item -ItemType Directory -Force -Path $distributionDirectory | Out-Null
$stagingDirectory = Join-Path $distributionDirectory ('stage-' + [guid]::NewGuid().ToString('N'))
$archive = Join-Path $distributionDirectory "BubbleFighter.Server-$Version-Windows-x64.zip"
$temporaryArchive = Join-Path $distributionDirectory ([guid]::NewGuid().ToString('N') + '.zip')
New-Item -ItemType Directory -Path $stagingDirectory | Out-Null
try {
    foreach ($file in $programFiles) {
        Copy-Item -LiteralPath (Join-Path $outputDirectory $file) -Destination $stagingDirectory
    }
    $runtimeFiles | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $stagingDirectory }
    Copy-Item -LiteralPath (Join-Path $repositoryRoot 'LICENSE') -Destination $stagingDirectory
    $configurationDirectory = Join-Path $stagingDirectory 'config/examples'
    $migrationDirectory = Join-Path $stagingDirectory 'database/migrations'
    $licenseDirectory = Join-Path $stagingDirectory 'licenses/mysql/docs'
    New-Item -ItemType Directory -Force -Path $configurationDirectory, $migrationDirectory, $licenseDirectory | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'config/examples') -Filter '*.ini' -File |
        ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $configurationDirectory }
    Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'src/persistence/migrations') -Filter '*.sql' -File |
        ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $migrationDirectory }
    foreach ($file in @('LICENSE', 'README', 'docs/INFO_BIN', 'docs/INFO_SRC')) {
        Copy-Item -LiteralPath (Join-Path $MySqlRoot $file) -Destination (Join-Path $stagingDirectory "licenses/mysql/$file")
    }
    $releaseInstructions = @"
BubbleFighter.Server $Version - Windows x64

Keep the executable and all DLLs in the same directory.
Show command-line help:
  .\BubbleFighter.Server.exe --help
Listen without a database for a basic connection check:
  .\BubbleFighter.Server.exe --listen-port 0 --once

Game accounts and persistent features require a local MySQL database with TLS.
Copy config/examples/database.ini to a private configuration file and edit it.
Apply only migrations that have not already run, in this order:
  create_local_accounts.sql
  create_characters.sql
  add_character_statistics.sql
  create_inventory_slots.sql
  create_cash_inventory_slots.sql
  create_weapon_presets.sql
  expand_cash_inventory_usage.sql
  create_match_settlements.sql
The scripts are in database/migrations; the program does not apply them.
Start the configured server, replacing the example path and port:
  .\BubbleFighter.Server.exe --listen-port 40000 --database-config .\Database.ini
Custom reward rules are in config/examples/match_rules.ini.

Build, configuration and client integration documentation:
https://github.com/K1roSama/BubbleFighter_KR1304_Server/tree/$Version/docs
An actual game client and its integration are required for live gameplay.

MySQL client/OpenSSL binaries are from the selected MySQL Windows distribution.
Build/source details and notices are in licenses/mysql. MSVC x64 CRT DLLs are included.
"@
    Set-Content -LiteralPath (Join-Path $stagingDirectory 'README.txt') -Value $releaseInstructions -Encoding utf8
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory($stagingDirectory, $temporaryArchive)
    Move-Item -LiteralPath $temporaryArchive -Destination $archive -Force
    $hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash *$(Split-Path -Leaf $archive)" | Set-Content -LiteralPath "$archive.sha256" -Encoding ascii
    Write-Output "Archive: $archive"
    Write-Output "SHA256: $hash"
}
finally {
    $stagingPath = [System.IO.Path]::GetFullPath($stagingDirectory)
    $distributionPath = [System.IO.Path]::GetFullPath($distributionDirectory) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $stagingPath.StartsWith($distributionPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'Refusing to clean a staging directory outside Artifacts/Distribution.'
    }
    Remove-Item -LiteralPath $stagingPath -Recurse -Force
}
