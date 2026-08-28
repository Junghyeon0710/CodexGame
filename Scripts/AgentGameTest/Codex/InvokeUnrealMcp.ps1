[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('list_toolsets', 'describe_toolset', 'call_tool')]
    [string]$MetaTool,

    [string]$ToolsetName = '',
    [string]$ToolName = '',
    [string]$ArgumentsJson = '{}',

    [ValidateRange(1, 20)]
    [int]$Repeat = 1
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
            name = 'CodexLastStandStep01QA'
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

switch ($MetaTool) {
    'list_toolsets' {
        $metaArguments = [pscustomobject]@{}
    }
    'describe_toolset' {
        if ([string]::IsNullOrWhiteSpace($ToolsetName)) {
            throw 'ToolsetName is required for describe_toolset.'
        }
        $metaArguments = [pscustomobject]@{ toolset_name = $ToolsetName }
    }
    'call_tool' {
        if ([string]::IsNullOrWhiteSpace($ToolsetName) -or [string]::IsNullOrWhiteSpace($ToolName)) {
            throw 'ToolsetName and ToolName are required for call_tool.'
        }
        $metaArguments = [pscustomobject]@{
            toolset_name = $ToolsetName
            tool_name = $ToolName
            arguments = ($ArgumentsJson | ConvertFrom-Json)
        }
    }
}

for ($callIndex = 0; $callIndex -lt $Repeat; ++$callIndex) {
    $callBody = @{
        jsonrpc = '2.0'
        id = 2 + $callIndex
        method = 'tools/call'
        params = @{
            name = $MetaTool
            arguments = $metaArguments
        }
    } | ConvertTo-Json -Depth 30 -Compress

    $callResponse = Invoke-WebRequest `
        -Uri $endpoint `
        -Method Post `
        -Headers $sessionHeaders `
        -ContentType 'application/json' `
        -Body $callBody

    $callResponse.Content
}
