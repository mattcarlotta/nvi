<#
.SYNOPSIS
    Installs the nvi CLI on Windows 10 or newer (Windows PowerShell 5.1 and PowerShell 7+).

.EXAMPLE
    irm https://raw.githubusercontent.com/mattcarlotta/nvi/main/install.ps1 | iex

.EXAMPLE
    .\install.ps1 -Version v0.1.2 -InstallDir "C:\tools\bin"

.EXAMPLE
    .\install.ps1 -Uninstall
#>
[CmdletBinding()]
param(
    [string] $Version = $(if ($env:NVI_VERSION) { $env:NVI_VERSION } else { 'latest' }),
    [string] $InstallDir = $(if ($env:NVI_INSTALL_DIR) { $env:NVI_INSTALL_DIR } else { "$env:LOCALAPPDATA\Programs\nvi\bin" }),
    [switch] $NoPathUpdate,
    [switch] $NoProfileUpdate,
    [switch] $Uninstall
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$Repo = 'mattcarlotta/nvi'
$Bin = 'nvi.exe'
$BeginMarker = '# >>> nvi >>>'
$EndMarker = '# <<< nvi <<<'

$NvixFn = 'function nvix { nvi @args | Out-String | Invoke-Expression }'

function Write-Info { param([string] $Message) Write-Host $Message }
function Fail { param([string] $Message) Write-Error $Message; exit 1 }

function Remove-FromUserPath {
    param([string] $Dir)
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    if (-not $userPath) { return }
    $kept = $userPath.Split(';') | Where-Object { $_ -and $_.TrimEnd('\') -ne $Dir.TrimEnd('\') }
    [Environment]::SetEnvironmentVariable('Path', ($kept -join ';'), 'User')
}

function Add-ToUserPath {
    param([string] $Dir)
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    $entries = @()
    if ($userPath) { $entries = @($userPath.Split(';') | Where-Object { $_ }) }
    if ($entries | Where-Object { $_.TrimEnd('\') -eq $Dir.TrimEnd('\') }) { return $false }
    [Environment]::SetEnvironmentVariable('Path', (($entries + $Dir) -join ';'), 'User')
    return $true
}

function Test-ProfileBlock {
    if (-not (Test-Path -LiteralPath $PROFILE)) { return $false }
    return (@(Get-Content -LiteralPath $PROFILE) -contains $BeginMarker)
}

function Add-ProfileBlock {
    param([string] $Block)
    $dir = Split-Path -Parent $PROFILE
    if ($dir -and -not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
    # UTF8 keeps a BOM on 5.1, which is what that host needs to read non-ASCII
    # profile content correctly. Its Set-Content default would be ANSI.
    Add-Content -LiteralPath $PROFILE -Value ([Environment]::NewLine + $Block) -Encoding UTF8
}

function Remove-ProfileBlock {
    if (-not (Test-ProfileBlock)) { return }
    $kept = New-Object System.Collections.Generic.List[string]
    $skipping = $false
    foreach ($line in @(Get-Content -LiteralPath $PROFILE)) {
        if ($line -eq $BeginMarker) { $skipping = $true; continue }
        if ($skipping) {
            if ($line -eq $EndMarker) { $skipping = $false }
            continue
        }
        $kept.Add($line)
    }
    Set-Content -LiteralPath $PROFILE -Value $kept -Encoding UTF8
    Write-Info "removed the nvi block from $PROFILE"
}

# ------------------------------------------------------------------ uninstall

if ($Uninstall) {
    $target = Join-Path $InstallDir $Bin
    if (-not (Test-Path -LiteralPath $target)) { Fail "no nvi found at $target" }
    Remove-Item -LiteralPath $target -Force
    Write-Info "removed $target"
    if (-not (Get-ChildItem -LiteralPath $InstallDir -Force)) {
        Remove-Item -LiteralPath $InstallDir -Force
    }
    Remove-FromUserPath -Dir $InstallDir
    Remove-ProfileBlock
    Write-Info 'Close and reopen PowerShell for the Path change to take effect.'
    exit 0
}

# --------------------------------------------------------------- environment

if ([Environment]::OSVersion.Version.Build -lt 10240) {
    Fail 'nvi requires Windows 10 or newer.'
}

$arch = $env:PROCESSOR_ARCHITECTURE
switch ($arch) {
    'AMD64' { }
    'ARM64' { Write-Warning 'Only an x86_64 build is published; it will run under Windows x64 emulation.' }
    default { Fail "unsupported architecture: $arch (only x86_64 binaries are published)" }
}

# Windows PowerShell 5.1 does not negotiate TLS 1.2 by default.
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
} catch { }

$headers = @{ 'User-Agent' = 'nvi-installer' }

# ------------------------------------------------------------------- version

if ($Version -eq 'latest') {
    try {
        $release = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/latest" -Headers $headers -UseBasicParsing
        $tag = $release.tag_name
    } catch {
        Fail "could not resolve the latest release ($($_.Exception.Message)); pass -Version <tag>"
    }
} elseif ($Version.StartsWith('v')) {
    $tag = $Version
} else {
    $tag = "v$Version"
}

$asset = 'nvi-windows-x86_64.zip'
$url = "https://github.com/$Repo/releases/download/$tag/$asset"

# ------------------------------------------------------------------- install

Write-Info "installing nvi $tag (windows-x86_64) -> $InstallDir"

$temp = Join-Path ([IO.Path]::GetTempPath()) ("nvi-install-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null

try {
    $zip = Join-Path $temp $asset
    try {
        Invoke-WebRequest -Uri $url -OutFile $zip -Headers $headers -UseBasicParsing
    } catch {
        Fail "download failed: $url ($($_.Exception.Message))"
    }

    Expand-Archive -LiteralPath $zip -DestinationPath $temp -Force

    $src = Join-Path $temp "nvi\bin\$Bin"
    if (-not (Test-Path -LiteralPath $src)) { Fail "archive did not contain nvi\bin\$Bin" }

    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    $dest = Join-Path $InstallDir $Bin

    if (Test-Path -LiteralPath $dest) {
        try {
            Remove-Item -LiteralPath $dest -Force
        } catch {
            Fail "$dest is in use or locked; close any running nvi processes and retry"
        }
    }

    Copy-Item -LiteralPath $src -Destination $dest -Force
    Unblock-File -LiteralPath $dest
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Info ''
& (Join-Path $InstallDir $Bin) version
if ($LASTEXITCODE -ne 0) { Fail 'installed binary failed to run' }
Write-Info ''

# ---------------------------------------------------------------- path notice

# A .ps1 runs in the calling session, so this Path assignment is live
# immediately. Only a new session needs the persisted user Path.
$onPath = @($env:Path -split ';' | Where-Object { $_ }) |
    Where-Object { $_.TrimEnd('\') -eq $InstallDir.TrimEnd('\') }
$not = if ($onPath) { '' } else { ' not' }

Write-Info "$InstallDir is$not a recognized path within your Path."

if ($NoPathUpdate) {
    Write-Info "Skipped the Path update. You MUST add $InstallDir to your Path manually."
} elseif (Add-ToUserPath -Dir $InstallDir) {
    if (-not $onPath) { $env:Path = "$env:Path;$InstallDir" }
    Write-Info "Added $InstallDir to your user Path."
} else {
    Write-Info "$InstallDir is already on your user Path."
}

# ------------------------------------------------------------- profile notice

$block = @($BeginMarker, $NvixFn, $EndMarker) -join [Environment]::NewLine

if ($NoProfileUpdate) {
    Write-Info ''
    Write-Info 'Add the following to your PowerShell profile (notepad $PROFILE):'
    Write-Info ''
    Write-Info (($block -split "`r?`n" | ForEach-Object { "    $_" }) -join [Environment]::NewLine)
} elseif (Test-ProfileBlock) {
    Write-Info ''
    Write-Info "$PROFILE already contains an nvi block, so it was left unchanged."
    Write-Info 'Run -Uninstall and reinstall to regenerate it.'
} else {
    try {
        Add-ProfileBlock -Block $block
    } catch {
        Fail "could not write to $PROFILE ($($_.Exception.Message))"
    }
    Write-Info ''
    Write-Info "Appended an nvi block to ${PROFILE}:"
    Write-Info ''
    Write-Info (($block -split "`r?`n" | ForEach-Object { "    $_" }) -join [Environment]::NewLine)
    Write-Info ''
    Write-Info 'Reload it in your current session (a new session picks it up automatically):'
    Write-Info ''
    Write-Info "    . `$PROFILE"
}
