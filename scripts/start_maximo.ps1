[CmdletBinding()]
param(
    [ValidateSet('4x3','16x9')]
    [string]$Profile = '4x3',

    [string]$BuildDirectory,

    [string]$QuickDirectory
)

$ErrorActionPreference = 'Stop'
if (!$BuildDirectory) {
    $BuildDirectory = Join-Path $PSScriptRoot "..\build\stage5-$Profile"
}
$manifest = Join-Path $BuildDirectory 'runner.json'
if (!(Test-Path -LiteralPath $manifest -PathType Leaf)) {
    throw "Build ausente em $manifest. Gere o perfil $Profile com scripts/build_maximo.py."
}
$runner = Get-Content -LiteralPath $manifest -Encoding UTF8 -Raw | ConvertFrom-Json
if ($runner.Count -lt 2 -or !(Test-Path -LiteralPath $runner[0] -PathType Leaf) -or
                            !(Test-Path -LiteralPath $runner[1] -PathType Leaf)) {
    throw "runner.json precisa apontar para o executável e o ELF local gerados por scripts/build_maximo.py."
}
if (($Profile -eq '16x9') -ne ($runner -contains '--widescreen')) {
    throw "O runner.json não corresponde ao perfil $Profile."
}
if ($Profile -eq '16x9') {
    $patchIndex = [Array]::IndexOf($runner, '--patches')
    if ($patchIndex -lt 0 -or $patchIndex + 1 -ge $runner.Count -or
        !(Test-Path -LiteralPath $runner[$patchIndex + 1] -PathType Leaf)) {
        throw "O launcher 16:9 precisa da lista de patches gerada com o ELF widescreen."
    }
}
$runnerArgs = @($runner[1..($runner.Count - 1)]) + '--window'
if ($QuickDirectory) {
    $saveDir = New-Item -ItemType Directory -Path $QuickDirectory -Force
    $runnerArgs += '--quick-dir', $saveDir.FullName
}
& $runner[0] @runnerArgs
