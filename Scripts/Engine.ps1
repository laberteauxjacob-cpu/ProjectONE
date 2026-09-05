function Resolve-ONEEngine {
    param([string]$EngineRoot = $env:UE_ROOT)
    if (!$EngineRoot) { $EngineRoot = Join-Path $env:ProgramFiles 'Epic Games\UE_5.7' }
    $resolved = [IO.Path]::GetFullPath($EngineRoot)
    if (!(Test-Path -LiteralPath (Join-Path $resolved 'Engine\Build\BatchFiles\Build.bat'))) {
        throw 'UE 5.7 was not found. Set UE_ROOT to the installed Unreal Engine directory or pass -EngineRoot.'
    }
    return $resolved
}
