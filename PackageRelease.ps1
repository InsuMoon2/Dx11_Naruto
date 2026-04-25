param(
    # Server IPv4 address written into the packaged client NetworkConfig.json.
    [string]$ServerIp = "127.0.0.1",

    # Server TCP port written into the packaged client NetworkConfig.json.
    [int]$Port = 7777,

    # Output folder that receives the external-PC release package.
    [string]$OutputDir = "Dist\ReleaseExternal"
)

$ErrorActionPreference = "Stop"

# Repository root used as the stable base for all relative copy paths.
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

# Absolute package root recreated on each packaging run.
$PackageRoot = Join-Path $RepoRoot $OutputDir

# Client executable folder inside the package.
$PackageGameBin = Join-Path $PackageRoot "Game\Bin"

# Server executable folder inside the package.
$PackageServerBin = Join-Path $PackageRoot "Server\GameServer\Bin"

# Client runtime data folder inside the package.
$PackageClientBin = Join-Path $PackageRoot "Client\Bin"

if (Test-Path $PackageRoot) {
    Remove-Item -LiteralPath $PackageRoot -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $PackageGameBin | Out-Null
New-Item -ItemType Directory -Force -Path $PackageServerBin | Out-Null
New-Item -ItemType Directory -Force -Path $PackageClientBin | Out-Null

# Runtime DLL/EXE files required beside Game.exe.
$GameRuntimeFiles = @(
    "Game\Bin\Game.exe",
    "Game\Bin\Engine.dll",
    "Game\Bin\assimp-vc143-mt.dll",
    "Game\Bin\assimp-vc143-mtd.dll",
    "Game\Bin\fmod.dll",
    "Game\Bin\fmodL.dll"
)

foreach ($RelativePath in $GameRuntimeFiles) {
    # Source file copied from the repository build output.
    $SourcePath = Join-Path $RepoRoot $RelativePath

    if (Test-Path $SourcePath) {
        Copy-Item -LiteralPath $SourcePath -Destination $PackageGameBin -Force
    }
}

# Game server executable used by the host PC.
$GameServerExe = Join-Path $RepoRoot "Server\GameServer\Bin\GameServer.exe"

if (Test-Path $GameServerExe) {
    Copy-Item -LiteralPath $GameServerExe -Destination $PackageServerBin -Force
}

# Full client resources are required because runtime paths resolve to ../../Client/Bin/Resources.
$SourceResources = Join-Path $RepoRoot "Client\Bin\Resources"

if (Test-Path $SourceResources) {
    Copy-Item -LiteralPath $SourceResources -Destination $PackageClientBin -Recurse -Force
}

# HLSL include/source folder kept for runtime shader fallback paths.
$SourceShaderFolder = Join-Path $RepoRoot "Client\Bin\Shaders"

if (Test-Path $SourceShaderFolder) {
    Copy-Item -LiteralPath $SourceShaderFolder -Destination $PackageClientBin -Recurse -Force
}

# Precompiled shader blobs loaded by the shader table.
$SourceShaderBlobs = Join-Path $RepoRoot "Client\Bin\Shader_*.cso"

Copy-Item -Path $SourceShaderBlobs -Destination $PackageClientBin -Force -ErrorAction SilentlyContinue

# Network config file that lets the packaged client target the host server PC.
$PackageNetworkConfig = Join-Path $PackageClientBin "Resources\Data\json\NetworkConfig.json"

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $PackageNetworkConfig) | Out-Null

@{
    server_ip = $ServerIp
    port = $Port
} | ConvertTo-Json | Set-Content -LiteralPath $PackageNetworkConfig -Encoding UTF8

# Zip file path produced beside the package directory.
$ZipPath = "$PackageRoot.zip"

if (Test-Path $ZipPath) {
    Remove-Item -LiteralPath $ZipPath -Force
}

Compress-Archive -Path (Join-Path $PackageRoot "*") -DestinationPath $ZipPath -Force

Write-Host "Package ready: $PackageRoot"
Write-Host "Zip ready: $ZipPath"
