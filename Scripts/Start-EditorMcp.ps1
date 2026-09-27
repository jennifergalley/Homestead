[CmdletBinding()]
param(
    [string]$EngineRoot,
    [int]$Port = 8765,
    [string]$Map,
    [string[]]$ExtraPlugins = @(),
    [switch]$AllowPython,
    [switch]$RayTracing,
    [switch]$SkipBuild,
    [int]$TimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'SurvivalGame.uproject'
$url = "http://127.0.0.1:$Port/mcp"

# Editor-only plugins shipped with UE 5.8 (Experimental). They are enabled for this editor
# process only, so the .uproject, commandlets, cooking and packaged builds are unaffected.
$plugins = @(
    'ModelContextProtocol'
    'EditorToolset'
    'AutomationTestToolset'
    'ConfigSettingsToolset'
    'LiveCodingToolset'
    'SlateInspectorToolset'
    'PluginToolset'
    'AnimationAssistantToolset'
    'PhysicsToolsets'
) + $ExtraPlugins

function Test-McpServer {
    $body = '{"jsonrpc":"2.0","id":1,"method":"ping"}'
    try {
        $null = Invoke-WebRequest -Uri $url -Method Post -Body $body -ContentType 'application/json' `
            -Headers @{ Accept = 'application/json, text/event-stream' } -TimeoutSec 3 -SkipHttpErrorCheck
        return $true
    } catch { return $false }
}

if (Test-McpServer) {
    Write-Host "Unreal MCP server is already answering at $url"
    return
}

$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')

if (-not $SkipBuild) {
    $build = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
    & $build SurvivalGameEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoXGE -NoFASTBuild
    if ($LASTEXITCODE -ne 0) { throw "Unreal editor-module build failed ($LASTEXITCODE)." }
}

$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$arguments = @("`"$project`"")
if ($Map) { $arguments += $Map }
$arguments += @(
    "-EnablePlugins=$($plugins -join ',')"
    '-ModelContextProtocolStartServer'
    "-ModelContextProtocolPort=$Port"
    # Agents drive the editor while another window has focus; don't throttle PIE in the background.
    '-ini:EditorSettings:[/Script/UnrealEd.EditorPerformanceSettings]:bThrottleCPUWhenNotForeground=False'
    # Live Coding's console group is shared across worktrees, so an active one blocks every other
    # worktree's SurvivalGameEditor build. Keep it off; rebuild and relaunch instead.
    '-ini:EditorPerProjectUserSettings:[/Script/LiveCoding.LiveCodingSettings]:bEnabled=False'
    '-nosplash'
) + @(& (Join-Path $PSScriptRoot 'Get-UnrealOfflineArguments.ps1'))
# Several agent editors share one GPU. Building ray-tracing pipelines in all of them at once has
# reset the driver (DXGI_ERROR_DEVICE_REMOVED / DRIVER_INTERNAL_ERROR), taking every Unreal process
# down with it, so agent editors render without ray tracing unless -RayTracing is passed.
if (-not $RayTracing) { $arguments += '-DPCVars=r.RayTracing.Enable=0' }
# Opt-in: registers homestead_agent.toolset.HomesteadEditorPython.run_python (arbitrary editor Python).
if ($AllowPython) { $arguments += '-HomesteadAgentPython' }

$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru
Write-Host "Started Unreal Editor (PID $($process.Id)); waiting for MCP at $url ..."

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $deadline) {
    if ($process.HasExited) { throw "Unreal Editor exited early with code $($process.ExitCode). See Saved\Logs\SurvivalGame.log." }
    if (Test-McpServer) {
        Write-Host "Unreal MCP server ready at $url (editor PID $($process.Id))."
        return
    }
    Start-Sleep -Seconds 3
}
throw "Unreal Editor is running (PID $($process.Id)) but MCP did not answer at $url within $TimeoutSeconds s."
