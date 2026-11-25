$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$runtime = Split-Path -Parent ([Drawing.Bitmap].Assembly.Location)
$references = @([Drawing.Bitmap].Assembly.Location, [Drawing.Color].Assembly.Location)
$references += @(Get-ChildItem -Path $runtime -Filter 'System.Private.Windows*.dll' | ForEach-Object FullName)
Add-Type -ReferencedAssemblies $references -Path (Join-Path $PSScriptRoot 'BuildMedieval.cs')
[BuildMedieval]::Generate((Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path)
