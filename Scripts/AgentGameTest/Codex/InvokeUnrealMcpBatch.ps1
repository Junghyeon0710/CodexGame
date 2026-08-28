[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BatchJson
)

$ErrorActionPreference = 'Stop'
$endpoint = 'http://127.0.0.1:8000/mcp'
$headers = @{ Accept = 'application/json, text/event-stream' }

$initializeBody = @{
    jsonrpc = '2.0'
    id = 1
    method = 'initialize'
    params = @{
        protocolVersion = '2025-11-25'
        capabilities = @{}
        clientInfo = @{
            name = 'CodexLastStandStep01BatchQA'
            version = '1.0'
        }
    }
} | ConvertTo-Json -Depth 20 -Compress

$initializeResponse = Invoke-WebRequest `
    -Uri $endpoint `
    -Method Post `
    -Headers $headers `
    -ContentType 'application/json' `
    -Body $initializeBody

$sessionId = [string]$initializeResponse.Headers['Mcp-Session-Id']
if ([string]::IsNullOrWhiteSpace($sessionId)) {
    throw 'MCP initialize response did not include Mcp-Session-Id.'
}

$sessionHeaders = @{
    Accept = 'application/json, text/event-stream'
    'Mcp-Session-Id' = $sessionId
}

$initializedBody = @{
    jsonrpc = '2.0'
    method = 'notifications/initialized'
    params = @{}
} | ConvertTo-Json -Depth 20 -Compress

Invoke-WebRequest `
    -Uri $endpoint `
    -Method Post `
    -Headers $sessionHeaders `
    -ContentType 'application/json' `
    -Body $initializedBody | Out-Null

$steps = @($BatchJson | ConvertFrom-Json)
$requestId = 2
foreach ($step in $steps) {
    if ($step.delayBeforeMilliseconds -gt 0) {
        Start-Sleep -Milliseconds ([int]$step.delayBeforeMilliseconds)
    }

    $callBody = @{
        jsonrpc = '2.0'
        id = $requestId
        method = 'tools/call'
        params = @{
            name = 'call_tool'
            arguments = @{
                toolset_name = [string]$step.toolsetName
                tool_name = [string]$step.toolName
                arguments = $step.arguments
            }
        }
    } | ConvertTo-Json -Depth 30 -Compress

    $callResponse = Invoke-WebRequest `
        -Uri $endpoint `
        -Method Post `
        -Headers $sessionHeaders `
        -ContentType 'application/json' `
        -Body $callBody

    $callResponse.Content
    ++$requestId
}
