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

```powershell
.\Scripts\Playtest\StopPackagedLocalSmoke.ps1
```

To review a still-running session without stopping it:

```powershell
.\Scripts\Playtest\ReviewInteractivePlaytest.ps1
```

Use `docs\plans\july-31-manual-vfx-acceptance.md` during the session. A clean automated log
review does not replace the owner/observer visual checks.

Useful log filters:

- `BB_BUILD`
- `BB_LOBBY`
- `BB_NETFLOW`
- `BB_DROP`
- `BB_MATCH`

## 3. LAN Test

On the host machine:

```powershell
.\Scripts\Playtest\StartPackagedServer.ps1
```

On another machine on the same network:

```powershell
.\Scripts\Playtest\StartPackagedClient.ps1 -Address HOST_LAN_IP:7777
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

After the handshake and full-roster ability smoke, verify that the package contains the required
executables, `TestMap`, all current Hudson cue packages, a 2/2 handshake, passing post-build smoke
evidence for all six hunters, and no critical network/runtime log errors:

```powershell
.\Scripts\Playtest\VerifyPlaytestCandidate.ps1
```

## Automated Packaged Ability Smoke

After the normal candidate verifier passes, drive all six hunters through LMB press/release, RMB
press/release, Shift, Q/recast, and R in three two-client packaged matches:

```powershell
.\Scripts\Playtest\TestPackagedFullRosterAbilitySmoke.ps1
```

This opt-in Development-build harness moves clients through lobby and hunter selection, places
opponents near each other, grants a large test health pool, records every activation, and rejects
crashes, failed grants, failed initial activations, missing releases, or incomplete cleanup.
Evidence is written under `Saved\Logs\PackagedAbilitySmoke`. It does not judge visual readability,
hitbox alignment, or effect quality; those remain in the manual acceptance matrix.
