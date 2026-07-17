param()

$ErrorActionPreference = "Stop"

$PowerShellCommand = Get-Command powershell.exe -ErrorAction Stop
$PowerShellPath = $PowerShellCommand.Source
$TestRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
	"BreachborneManualLauncherContract-{0}" -f [guid]::NewGuid().ToString("N"))
$SessionDirectory = Join-Path $TestRoot "RecordValidation"

$LauncherCases = @(
	@{
		Name = "Ghost/Eluna"
		Script = "StartGhostElunaAcceptance.ps1"
		Port = 17911
		Expected = "Ghost/Eluna acceptance launcher validation: PASS"
	},
	@{
		Name = "Kingpin/Hudson"
		Script = "StartKingpinHudsonAcceptance.ps1"
		Port = 17912
		Expected = "Kingpin/Hudson acceptance launcher validation: PASS"
	},
	@{
		Name = "Crysta/Void"
		Script = "StartCrystaVoidAcceptance.ps1"
		Port = 17913
		Expected = "Crysta/Void acceptance launcher validation: PASS"
	},
	@{
		Name = "Range indicators"
		Script = "StartRangeIndicatorAcceptance.ps1"
		Port = 17914
		Expected = "Range-indicator acceptance launcher validation: PASS"
	},
	@{
		Name = "Cleanup/stress normal"
		Script = "StartCleanupStressAcceptance.ps1"
		Port = 17915
		Expected = "Cleanup/stress acceptance launcher validation: PASS"
	},
	@{
		Name = "Cleanup/stress impaired"
		Script = "StartCleanupStressAcceptance.ps1"
		Port = 17916
		Expected = "Cleanup/stress acceptance launcher validation: PASS"
		ExtraArgs = @("-PktLag", "100", "-PktLoss", "2")
	},
	@{
		Name = "Focused Eluna repair"
		Script = "StartElunaRepairRetest.ps1"
		Port = 17917
		Expected = "Eluna repair-retest launcher validation: PASS"
	}
)

$BatchNames = @(
	"GhostEluna",
	"KingpinHudson",
	"CrystaVoid",
	"CleanupStress",
	"RangeIndicators",
	"WispUI"
)

function Get-BreachborneProcessIds {
	return @(Get-Process -ErrorAction SilentlyContinue | Where-Object {
		$_.ProcessName -match '^(UnrealEditor|UnrealEditor-Cmd|Breachborne|BreachborneServer)$'
	} | ForEach-Object { [int]$_.Id } | Sort-Object -Unique)
}

$Failures = @()
$PassedLaunchers = 0
$PassedRecords = 0

try {
	foreach ($Case in $LauncherCases) {
		$CaseOutputRoot = Join-Path $TestRoot ($Case.Name -replace '[^A-Za-z0-9]+', '-')
		$ScriptPath = Join-Path $PSScriptRoot $Case.Script
		$BeforeProcessIds = @(Get-BreachborneProcessIds)
		$InvocationArgs = @(
			"-NoProfile",
			"-ExecutionPolicy", "Bypass",
			"-File", $ScriptPath,
			"-ValidateOnly",
			"-Port", [string]$Case.Port,
			"-OutputRoot", $CaseOutputRoot
		)
		if ($Case.ExtraArgs) {
			$InvocationArgs += @($Case.ExtraArgs)
		}

		$Output = (& $PowerShellPath @InvocationArgs 2>&1 | Out-String)
		$ExitCode = $LASTEXITCODE
		Start-Sleep -Milliseconds 200
		$AfterProcessIds = @(Get-BreachborneProcessIds)
		$NewProcessIds = @($AfterProcessIds | Where-Object { $BeforeProcessIds -notcontains $_ })

		$CaseFailures = @()
		if ($ExitCode -ne 0) {
			$CaseFailures += "exit code $ExitCode"
		}
		if ($Output -notlike "*$($Case.Expected)*") {
			$CaseFailures += "missing PASS marker '$($Case.Expected)'"
		}
		if ($Output -notlike "*No session record or process was created.*") {
			$CaseFailures += "missing no-process marker"
		}
		$OutputArtifacts = if (Test-Path -LiteralPath $CaseOutputRoot) {
			@(Get-ChildItem -LiteralPath $CaseOutputRoot -Recurse -Force)
		} else {
			@()
		}
		if ($OutputArtifacts.Count) {
			$CaseFailures += "created session artifacts below '$CaseOutputRoot'"
		}
		if ($NewProcessIds.Count) {
			$CaseFailures += "created Unreal/game process IDs: $($NewProcessIds -join ', ')"
		}

		if ($CaseFailures.Count) {
			$Failures += "$($Case.Name): $($CaseFailures -join '; ')"
			Write-Host "Launcher contract: FAIL - $($Case.Name)"
			Write-Host $Output.TrimEnd()
		} else {
			$PassedLaunchers++
			Write-Host "Launcher contract: PASS - $($Case.Name)"
		}
	}

	New-Item -ItemType Directory -Path $SessionDirectory -Force | Out-Null
	foreach ($BatchName in $BatchNames) {
		$RecordScript = Join-Path $PSScriptRoot "NewManualAcceptanceRecord.ps1"
		$Output = (& $PowerShellPath `
			-NoProfile `
			-ExecutionPolicy Bypass `
			-File $RecordScript `
			-SessionDirectory $SessionDirectory `
			-Batch $BatchName `
			-ValidateOnly 2>&1 | Out-String)
		$ExitCode = $LASTEXITCODE
		$ExpectedRecordPath = Join-Path $SessionDirectory "$BatchName-ManualAcceptance.md"
		if ($ExitCode -ne 0 -or
			$Output -notlike "*Manual acceptance record validation: PASS*" -or
			(Test-Path -LiteralPath $ExpectedRecordPath)) {
			$Failures += "$BatchName record generator failed validation or wrote a record."
			Write-Host "Record contract: FAIL - $BatchName"
			Write-Host $Output.TrimEnd()
		} else {
			$PassedRecords++
			Write-Host "Record contract: PASS - $BatchName"
		}
	}
} finally {
	if (Test-Path -LiteralPath $TestRoot) {
		Remove-Item -LiteralPath $TestRoot -Recurse -Force
	}
}

if ($Failures.Count) {
	Write-Host "Manual acceptance launcher contracts: FAIL"
	$Failures | ForEach-Object { Write-Host "- $_" }
	exit 1
}

Write-Host "Manual acceptance launcher contracts: PASS"
Write-Host "  Launcher cases: $PassedLaunchers/$($LauncherCases.Count)"
Write-Host "  Record batches: $PassedRecords/$($BatchNames.Count)"
Write-Host "  Session artifacts created: 0"
Write-Host "  Unreal/game processes created: 0"
$global:LASTEXITCODE = 0
