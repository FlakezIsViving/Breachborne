param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$Address = "127.0.0.1:7777",
	[int]$ClientCount = 2,
	[string]$UnrealEditorExe = "C:\UnrealEngine-5.7.4-release\Engine\Binaries\Win64\UnrealEditor.exe"
)

$ResolvedProject = Resolve-Path -LiteralPath $ProjectPath

if (-not (Test-Path -LiteralPath $UnrealEditorExe)) {
	throw "UnrealEditor.exe not found at '$UnrealEditorExe'. Pass -UnrealEditorExe or update the script default."
}

for ($i = 0; $i -lt $ClientCount; $i++) {
	$X = 80 + ($i * 40)
	$Y = 80 + ($i * 40)
	Start-Process -FilePath $UnrealEditorExe -ArgumentList @(
		$ResolvedProject,
		$Address,
		"-game",
		"-log",
		"-windowed",
		"-ResX=1280",
		"-ResY=720",
		"-WinX=$X",
		"-WinY=$Y",
		"-nosteam"
	)
}
