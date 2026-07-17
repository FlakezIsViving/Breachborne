# Breachborne Packaged Playtest

This folder is for testing packaged builds outside PIE.

## 1. Package Client And Server

From the project root:

```powershell
.\Scripts\Packaging\PackagePlaytestWindows.ps1
```

Dedicated server packaging requires an Unreal Engine build that supports server targets. If the Epic-installed engine reports `Server targets are not currently supported from this engine distribution`, use a source-built/server-capable UE 5.7 install and pass its UAT path:

```powershell
.\Scripts\Packaging\PackagePlaytestWindows.ps1 -RunUAT "D:\UE_5.7_Source\Engine\Build\BatchFiles\RunUAT.bat"
```

To package only the Windows client from the Epic-installed engine:

```powershell
.\Scripts\Packaging\PackagePlaytestWindows.ps1 -SkipServer
```

Outputs:

- `Builds\WindowsClient`
- `Builds\WindowsServer`
- `Builds\PlaytestBuildSummary.txt`

The first pass uses `Development` builds so console commands and logs remain available.

## 2. Local Packaged Smoke Test

```powershell
.\Scripts\Playtest\StartPackagedLocalSmoke.ps1 -ClientCount 2
```

Expected flow:

1. Server boots `TestMap`.
2. Two clients connect to `127.0.0.1:7777`.
3. Clients enter custom lobby.
4. Move one client to another team.
5. Owner starts match.
6. Hunter select appears.
7. Players spawn/drop and can move.

The launcher starts the dedicated server hidden, opens clients in a side-by-side grid, records
the exact process IDs, and writes isolated logs under
`Saved\Logs\InteractivePlaytest\<timestamp>`. Stop only that recorded session and run the
automatic log review with:

It refuses to open a second recorded session while any process from the previous one is alive.
Hunter-pair acceptance wrappers also run `VerifyPlaytestCandidate.ps1` before opening windows, so
a stale or incomplete package fails before consuming manual test time.

Validate candidate freshness, executable discovery, stale-session state, and port availability
without opening anything:

```powershell
.\Scripts\Playtest\StartPackagedLocalSmoke.ps1 -ClientCount 3 -Port 7911 `
  -Address '127.0.0.1:7911' -VerifyCandidate -ValidateOnly
```

```powershell
.\Scripts\Playtest\StopPackagedLocalSmoke.ps1
```

To review a still-running session without stopping it:

```powershell
.\Scripts\Playtest\ReviewInteractivePlaytest.ps1
```

Use `docs\plans\july-31-manual-vfx-acceptance.md` during the session. A clean automated log
review does not replace the owner/observer visual checks.

Use `-SessionLabel` to identify a recorded batch. For manual impairment checks, pass the same
network simulation command to server and clients so `Session.json` preserves the exact setup:

```powershell
.\Scripts\Playtest\StartPackagedLocalSmoke.ps1 `
  -ClientCount 2 `
  -SessionLabel "Manual impairment retest" `
  -ServerExtraArgs '-ExecCmds="Net PktLag=100,Net PktLoss=2"' `
  -ClientExtraArgs '-ExecCmds="Net PktLag=100,Net PktLoss=2"'
```

Ghost/Eluna needs a third allied hunter for Eluna's pass-through, healing, living-target R, and
wisp flow. Launch its exact recorded topology with:

```powershell
.\Scripts\Playtest\StartGhostElunaAcceptance.ps1
```

The wrapper opens Eluna, an allied Hudson helper, and an opposing Ghost client, then writes the
required team/slot/hunter roles to `GhostElunaInstructions.txt` inside the session evidence
directory. Each pair launcher also creates a session-local `*-ManualAcceptance.md` record with
separate owner, observer, gameplay, cleanup, result, and notes fields. Stop it with the normal
`StopPackagedLocalSmoke.ps1` command.

For the post-repair Eluna RMB/Shift/Q retest, use the shorter wrapper instead of repeating all 12
Ghost/Eluna rows:

```powershell
.\Scripts\Playtest\StartElunaRepairRetest.ps1
```

It creates `ElunaRepairRetestInstructions.txt` and a three-row `ElunaRepairRetest.md` result record.
Run it with `-ValidateOnly` to check paths, stale-session state, and the port without opening windows.
The real launch performs the candidate-freshness gate before it starts any process.

The structured definition covers all 55 presentation rows, including cleanup/stress, range, and
wisp UI. Verify that it still matches both checklist documents with:

```powershell
.\Scripts\Playtest\TestManualAcceptanceDefinition.ps1
```

Validate every manual launcher and ledger generator without starting Unreal or writing a session:

```powershell
.\Scripts\Playtest\TestManualAcceptanceLaunchers.ps1
```

The launcher contract covers normal and 100 ms/2% loss cleanup modes, compares Unreal/game process
IDs before and after every case, rejects session artifacts, and validates all six record batches in
an isolated temporary folder.

Create a global-row record inside any relevant session without launching another process:

```powershell
.\Scripts\Playtest\NewManualAcceptanceRecord.ps1 `
  -SessionDirectory 'Saved\Logs\InteractivePlaytest\SESSION' `
  -Batch WispUI
```

The remaining hunter-pair launchers are:

```powershell
.\Scripts\Playtest\StartKingpinHudsonAcceptance.ps1
.\Scripts\Playtest\StartCrystaVoidAcceptance.ps1
```

Append `-ValidateOnly` to any pair launcher to check executable discovery, the 55-row definition,
the stale-session guard, and assigned UDP port without opening clients or creating a session record.
The real launch additionally requires the current candidate and post-build evidence to verify.

Kingpin/Hudson opens a third enemy target so the chain visual is testable. Crysta/Void uses a placed
target dummy as the marked-prop fixture; the visual prop-swap row still requires matching owner and
observer endpoints and must not be credited from deterministic tests alone.

Use `StartRangeIndicatorAcceptance.ps1` for the owner-only range matrix. It launches four clients,
creates the `RangeIndicators` record, and supplies a two-game six-hunter plan plus built-in F/G power
console commands. Real launch still requires a current verified package; use `-ValidateOnly` only to
check launcher topology without starting processes.

Use `StartCleanupStressAcceptance.ps1` for CS-01 through CS-05. Run it once normally for death cleanup,
Low/Medium quality, and Grappling Hook, then once with `-PktLag 100 -PktLoss 2` for the impaired Hudson
and persistent-zone row. Each session gets its own `CleanupStress` record and reviewed logs. The
Ghost/Eluna wrapper creates both `GhostEluna` and `WispUI` records because those checks share its
three-client ally/enemy topology.

Useful log filters:

- `BB_BUILD`
- `BB_LOBBY`
- `BB_NETFLOW`
- `BB_DROP`
- `BB_MATCH`

## 3. LAN Test

Use `docs\plans\july-31-direct-ip-acceptance.md` for the authoritative second-machine gate and
result fields. The launchers below now write timestamped logs and executable hashes instead of
relying on the package's default log folder.

## VFX Foundation Audit

Distinguish a primitive-fallback-ready candidate from completed Niagara authoring with:

```powershell
.\Scripts\Playtest\AuditVfxFoundation.ps1
```

The report is written to `Builds/VfxFoundationAudit.txt`. Missing master `.uasset` files are
reported explicitly without failing the primitive fallback baseline; missing cue roots, template
contract, scalability, or primitive fallback files do fail the audit. Any project `NS_*.uasset`
outside the eight canonical paths is listed separately as an inventory-only candidate and never
increments the authored-master count. Once a canonical file exists and Unreal Editor is closed, the
audit runs `BBNiagaraMasterAudit` to load the asset and verify its class, required typed `User.*`
parameters, enabled CPU emitters, system fixed bounds, and absence of enabled light renderers. A
present asset that fails those checks makes the audit fail while primitive fallbacks remain usable.
Particle budgets, culling, cleanup, geometry binding, and top-down readability still require the
editor/manual review described in `docs/plans/ludus-vfx-prompt-pack.md`.

## Distribution Preparation

Validate release inputs without copying or recompressing the frozen candidate:

```powershell
.\Scripts\Packaging\PreparePlaytestDistribution.ps1 -IncludeServer -ValidateOnly
```

This also runs the 55-row definition check, distribution verifier 8/8 self-test, and direct-IP
reviewer 4/4 self-test before reporting readiness.

After the final candidate passes manual gates, omit `-ValidateOnly` to stage a client bundle and
optional server bundle under `Builds/Distribution`. The bundles preserve `Builds`, `Scripts`, and
`docs` relative paths, include candidate/direct-IP evidence, and contain per-file
`SHA256SUMS.txt`. On each destination machine, run
`.\Scripts\Playtest\VerifyPlaytestDistribution.ps1` from the bundle root before the client/server
launcher. Verification rejects missing, changed, duplicated, out-of-root, and unexpected files.
Add `-CreateArchives` only when final transfer archives are needed.

Validate those rejection paths using an isolated temporary fixture under `Saved/Logs`:

```powershell
.\Scripts\Playtest\TestDistributionVerifier.ps1
```

On the host machine:

```powershell
.\Scripts\Playtest\StartPackagedServer.ps1
```

On another machine on the same network:

```powershell
.\Scripts\Playtest\StartPackagedClient.ps1 -Address HOST_LAN_IP:7777
```

Run one host client as well, because the manual gate requires host/remote observation. Use
`StopPackagedClient.ps1` on the remote machine for the reconnect step, then collect both remote
session directories and run `ReviewDirectIPPlaytest.ps1` on the host. The authoritative topology
and commands are in `docs\plans\july-31-direct-ip-acceptance.md`.

The reviewer and six stable `DI` rows have an isolated no-network self-test:

```powershell
.\Scripts\Playtest\TestDirectIPReviewer.ps1
```

## 4. Remote Direct-IP Test

1. Copy `Builds\WindowsServer` and `Scripts\Playtest\StartPackagedServer.ps1` to the Windows VPS.
2. Open inbound UDP `7777` in Windows Firewall and the cloud firewall/security group.
3. Run:

   ```powershell
   .\StartPackagedServer.ps1
   ```

4. Send testers `Builds\WindowsClient` and `Scripts\Playtest\StartPackagedClient.ps1`.
5. Testers run:

   ```powershell
   .\StartPackagedClient.ps1 -Address PUBLIC_IP:7777
   ```

## 5. Logs

Packaged logs are written under each packaged build's `Saved\Logs` folder.

For bug reports, ask testers to send:

- client log from `WindowsClient\...\Saved\Logs`
- server log from `WindowsServer\...\Saved\Logs`
- what they were doing
- visible error or screenshot

## 6. Firewall Note

Unreal direct IP uses UDP `7777` by default. Windows may prompt on first launch. Allow private network for LAN tests and public network for VPS/remote tests.
## Headless Packaged Handshake

After packaging both targets, validate that two packaged clients can join the packaged dedicated
server without opening windows:

```powershell
.\Scripts\Playtest\TestPackagedHeadlessHandshake.ps1 -ClientCount 2
```

The script launches the inner packaged binaries directly, captures separate logs, validates the
server welcomes and joins, and terminates only the processes it started. Evidence is written under
`Saved/Logs/PackagedHandshake/<timestamp>`.

After the handshake and smoke runs, verify source freshness plus required executables, `TestMap`,
all current Hudson cue packages, a 2/2 handshake, normal/impaired post-build evidence for all six
hunters, reconnect-attempt evidence, death/wisp/revive evidence, the wisp-priority matrix, four-client
transport plus 2v2 lobby/selection/ability evidence, and clean runtime log reviews:

```powershell
.\Scripts\Playtest\VerifyPlaytestCandidate.ps1
```

After any gameplay/content/config/plugin rebuild, refresh every required evidence gate and then run
the verifier with one sequential command:

```powershell
.\Scripts\Playtest\RefreshPackagedCandidateEvidence.ps1
```

The refresh deliberately runs sequentially. Parallel headless launches can starve client startup,
and the two death directions must not create evidence directories within the same second. Use
`-BasePort` when the default isolated range `8500-8610` is unavailable. Use `-VerifyOnly` to check
existing evidence without launching tests.

## Automated Packaged Ability Smoke

After packaging, drive all six hunters through LMB press/release, RMB
press/release, Shift, Q/recast, and R in three two-client packaged matches:

```powershell
.\Scripts\Playtest\TestPackagedFullRosterAbilitySmoke.ps1
```

This opt-in Development-build harness moves clients through lobby and hunter selection, places
opponents near each other, grants a large test health pool, records every activation, and rejects
crashes, failed grants, failed initial activations, missing releases, or incomplete cleanup.
Evidence and automatic critical/cue-overflow reviews are written under
`Saved\Logs\PackagedAbilitySmoke`. It does not judge visual readability, hitbox alignment, or
effect quality; those remain in the manual acceptance matrix.

Run the same six-hunter activation lifecycle under the playtest packet impairment settings:

```powershell
.\Scripts\Playtest\TestPackagedNetworkImpairment.ps1
```

This applies `Net PktLag=100` and `Net PktLoss=2` to the server and both clients. The gate requires
all three roster matches to pass and every process log to confirm both settings. Evidence is
written under `Saved\Logs\PackagedNetworkImpairment`. This remains an activation test; the manual
persistent-zone overlap and visual-readability checks are separate.

Exercise one disconnect/reconnect attempt after a packaged two-player match reaches gameplay:

```powershell
.\Scripts\Playtest\TestPackagedReconnectAttempt.ps1
```

The runtime persists a random reconnect token in the client's user config; command-line overrides
keep same-machine test clients distinct. The script gives the second client an override token, terminates it after gameplay starts,
and requires the replacement process to reclaim the same team, lobby slot, hunter, transform,
health, shield, and inventory within the server's 60-second grace period. Unidentified late joins
still enter as spectators. Evidence is written under `Saved\Logs\PackagedReconnect`, and the run
also emits the critical/cue-overflow review required by the candidate verifier. Active casts,
temporary effects, and cooldown timers are deliberately reset rather than serialized.

Verify the authoritative lethal-damage to wisp-possession path in a packaged two-client match:

```powershell
.\Scripts\Playtest\TestPackagedDeathWispSmoke.ps1
.\Scripts\Playtest\TestPackagedDeathWispSmoke.ps1 -HunterA 2 -HunterB 1 -Port 7842
```

The opt-in test completes the selected hunters' normal ability lifecycle, injects a long-lived
hunter-owned sentinel plus Crysta/Void charge states, and applies lethal damage through the shared
GAS damage effect. It requires HealthSet depletion, owner-actor and transient-state cleanup, server
wisp spawn/possession, replicated victim-client wisp possession, repeated healing through the
capped revive path, one restarted server passive, and victim-client hunter repossession. The first
command tests the default Ghost-kills-Kingpin path; the second swaps the victim to cover Ghost's
passive teardown. Evidence is under `Saved\Logs\PackagedDeathWisp`.

Verify the complete wisp decay/revive priority matrix in a packaged dedicated match:

```powershell
.\Scripts\Playtest\TestPackagedWispRulesSmoke.ps1
```

Eluna downs Ghost, then the server measures natural decay, ally-only revive, ally/enemy contest,
enemy-only accelerated drain, healing-only accelerated revive, healing under enemy contest, the
carried-wisp contest exception, actual Eluna passive pickup, and CC-triggered drop. The test then
finishes a normal healing revive and requires victim-client hunter re-possession. Evidence and a
clean three-log review are written under `Saved\Logs\PackagedWispRules`. This proves gameplay
priority and replication lifecycle, not that the two bars are visible or readable on screen.

Require a real authoritative LMB hit and health decrease from every hunter:

```powershell
.\Scripts\Playtest\TestPackagedFullRosterHitSmoke.ps1
```

This runs three sequential two-client matches, repositions opponents after the normal activation
lifecycle, fires each hunter at close range, and requires server-observed health loss. Evidence is
written under `Saved\Logs\PackagedHitSmoke`. Other abilities and visual hitbox agreement remain in
the manual matrix.

Verify authoritative RMB, Shift, Q, and R outcomes for every hunter:

```powershell
.\Scripts\Playtest\TestPackagedFullRosterOutcomeSmoke.ps1
```

This runs six isolated two-client matches and requires 24/24 hunter-specific contracts covering
damage, healing, movement, crowd control, marks, empowered state, spawned actors, pulls, swaps,
and recast detonation. Every run also requires a clean critical-error and GameplayCue-overflow
review. It validates gameplay outcomes, not visual readability or owner/observer presentation.

Repeat the same 24 contracts with the playtest impairment profile:

```powershell
.\Scripts\Playtest\TestPackagedOutcomeNetworkImpairment.ps1
```

The wrapper applies `Net PktLag=100` and `Net PktLoss=2` to every server and client, requires all
18 process logs to confirm both settings, and writes separate evidence under
`Saved\Logs\PackagedOutcomeNetworkImpairment`.

Verify that the packaged dedicated server accepts four simultaneous client transports:

```powershell
.\Scripts\Playtest\TestPackagedFourClientHandshake.ps1
```

This requires four welcomes/joins and a clean five-log review under
`Saved\Logs\PackagedFourClientHandshake`. It does not prove 2v2 lobby assignment or combat.

Verify that four clients occupy the intended 2v2 slots, select unique hunters, enter gameplay, and
complete their standard ability lifecycles:

```powershell
.\Scripts\Playtest\TestPackagedFourClientAbilitySmoke.ps1
```

This assigns hunters 1-2 to team 0 slots 0-1 and hunters 3-4 to team 1 slots 0-1. The gate requires
4/4 exact server mappings, 20/20 primary activations, 8/8 LMB/RMB releases, four completed client
lifecycles, and a clean five-log review under `Saved\Logs\PackagedFourClientAbilitySmoke`. It proves
packaged 2v2 lobby, selection, spawn, and ability flow; deliberate movement, combat, and visual
readability remain in the manual acceptance matrix.

The wrapper waits for each indexed client to join before launching the next one. This preserves
deterministic auto-fill order and prevents simultaneous clients from occupying each other's
intended Team 0 slots before their explicit 2v2 slot requests arrive.
