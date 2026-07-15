param(
	[int]$BasePort = 8500,
	[switch]$VerifyOnly
)

$ErrorActionPreference = "Stop"

if ($BasePort -lt 1024 -or $BasePort + 110 -gt 65535) {
	throw "BasePort must leave room for the complete isolated port range ($BasePort-$($BasePort + 110))."
}

function Invoke-Gate([string]$Name, [scriptblock]$Action) {
	Write-Host ""
	Write-Host "=== $Name ==="
	$global:LASTEXITCODE = 0
	& $Action
	if ($LASTEXITCODE -ne 0) {
		throw "$Name failed with exit code $LASTEXITCODE."
	}
}

$VerifyScript = Join-Path $PSScriptRoot "VerifyPlaytestCandidate.ps1"
if ($VerifyOnly) {
	Invoke-Gate "Candidate verification" { & $VerifyScript }
	exit 0
}

Invoke-Gate "Packaged handshake" {
	& (Join-Path $PSScriptRoot "TestPackagedHeadlessHandshake.ps1") `
		-Port $BasePort `
		-ClientCount 2
}

Invoke-Gate "Full-roster ability lifecycle" {
	& (Join-Path $PSScriptRoot "TestPackagedFullRosterAbilitySmoke.ps1") `
		-BasePort ($BasePort + 10)
}

Invoke-Gate "Persistent match reset" {
	& (Join-Path $PSScriptRoot "TestPackagedMatchResetSmoke.ps1") `
		-Port ($BasePort + 20)
}

Invoke-Gate "Impaired full-roster ability lifecycle" {
	& (Join-Path $PSScriptRoot "TestPackagedNetworkImpairment.ps1") `
		-BasePort ($BasePort + 30)
}

Invoke-Gate "Reconnect restoration" {
	& (Join-Path $PSScriptRoot "TestPackagedReconnectAttempt.ps1") `
		-Port ($BasePort + 40)
}

Invoke-Gate "Death/wisp/revive: hunter 1 kills hunter 2" {
	& (Join-Path $PSScriptRoot "TestPackagedDeathWispSmoke.ps1") `
		-HunterA 1 `
		-HunterB 2 `
		-Port ($BasePort + 50)
}

# Evidence directories use second-resolution names. Keep opposite directions serialized.
Start-Sleep -Seconds 2
Invoke-Gate "Death/wisp/revive: hunter 2 kills hunter 1" {
	& (Join-Path $PSScriptRoot "TestPackagedDeathWispSmoke.ps1") `
		-HunterA 2 `
		-HunterB 1 `
		-Port ($BasePort + 51)
}

Invoke-Gate "Wisp lifecycle rules" {
	& (Join-Path $PSScriptRoot "TestPackagedWispRulesSmoke.ps1") `
		-Port ($BasePort + 60)
}

Invoke-Gate "Full-roster LMB hits" {
	& (Join-Path $PSScriptRoot "TestPackagedFullRosterHitSmoke.ps1") `
		-BasePort ($BasePort + 70)
}

Invoke-Gate "Full-roster authoritative outcomes" {
	& (Join-Path $PSScriptRoot "TestPackagedFullRosterOutcomeSmoke.ps1") `
		-BasePort ($BasePort + 80)
}

Invoke-Gate "Impaired authoritative outcomes" {
	& (Join-Path $PSScriptRoot "TestPackagedOutcomeNetworkImpairment.ps1") `
		-BasePort ($BasePort + 90)
}

Invoke-Gate "Four-client transport" {
	& (Join-Path $PSScriptRoot "TestPackagedFourClientHandshake.ps1") `
		-Port ($BasePort + 100)
}

Invoke-Gate "Four-client 2v2 ability flow" {
	& (Join-Path $PSScriptRoot "TestPackagedFourClientAbilitySmoke.ps1") `
		-Port ($BasePort + 110)
}

Invoke-Gate "Candidate verification" { & $VerifyScript }

Write-Host ""
Write-Host "Packaged candidate evidence refresh: PASS"
