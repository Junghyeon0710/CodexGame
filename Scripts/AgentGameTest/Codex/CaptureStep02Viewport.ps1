[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$absoluteOutput = [System.IO.Path]::GetFullPath((Join-Path $projectRoot $OutputPath))
if (-not $absoluteOutput.StartsWith($projectRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Output path must remain inside the project: $absoluteOutput"
}

$outputDirectory = Split-Path -Parent $absoluteOutput
if (-not (Test-Path -LiteralPath $outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory | Out-Null
}

$responseText = & (Join-Path $PSScriptRoot 'InvokeUnrealMcp.ps1') `
    -MetaTool call_tool `
    -ToolsetName 'EditorToolset.EditorAppToolset' `
    -ToolName 'CaptureEditorImage' `
    -ArgumentsJson '{}'

$response = $responseText | ConvertFrom-Json
if ($response.result.content[0].type -ne 'text') {
    throw 'CaptureViewport did not return a text envelope.'
}

$payload = $response.result.content[0].text | ConvertFrom-Json
$image = $payload.returnValue
if ([string]::IsNullOrWhiteSpace([string]$image.data)) {
    throw 'CaptureViewport returned no image data.'
}

[System.IO.File]::WriteAllBytes(
    $absoluteOutput,
    [System.Convert]::FromBase64String([string]$image.data))

[pscustomobject]@{
    path = $absoluteOutput
    mimeType = [string]$image.mimeType
    bytes = (Get-Item -LiteralPath $absoluteOutput).Length
} | ConvertTo-Json -Depth 8
