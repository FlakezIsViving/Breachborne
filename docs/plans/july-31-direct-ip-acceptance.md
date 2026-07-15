# July 31 Second-Machine Direct-IP Acceptance

This is a separate external-hardware gate. Localhost, four local clients, and headless automation
do not satisfy it.

## Preconditions

1. Use the verified package recorded by `Builds/PlaytestCandidateVerification.txt`.
2. Transfer either the staged `Client` bundle or `Builds/WindowsClient` plus its start/stop launchers to the
   second Windows machine without rebuilding it there.
3. Keep either the staged `Server` bundle or `Builds/WindowsServer` plus its launcher on the host.
4. Allow inbound UDP `7777` for `BreachborneServer.exe` in Windows Firewall on the host. For an
   internet/VPS test, also allow UDP `7777` in the router or cloud security group.
5. Choose one route and record it: same-LAN IPv4, VPN IPv4, or public IPv4. Do not use
   `127.0.0.1`, and do not build matchmaking for this gate.

When using a staged distribution bundle, run this from the bundle root to verify every transferred
file before launch:

```powershell
.\Scripts\Playtest\VerifyPlaytestDistribution.ps1
```

Do not continue if `DistributionVerification.txt` reports a missing file or hash mismatch.

Useful host address command:

```powershell
Get-NetIPAddress -AddressFamily IPv4 |
  Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.254.*' } |
  Select-Object InterfaceAlias,IPAddress,PrefixLength
```

## Run From Staged Bundles

On the host, from the staged `Server` bundle root:

```powershell
.\Scripts\Playtest\StartPackagedServer.ps1 -BuildRoot '.\Builds\WindowsServer' -Port 7777
```

In a second terminal on the host, launch the host player's client from the repository or staged
client bundle:

```powershell
.\Scripts\Playtest\StartPackagedClient.ps1 -Address '127.0.0.1:7777'
```

On the second machine, from the staged `Client` bundle root:

```powershell
.\Scripts\Playtest\StartPackagedClient.ps1 -BuildRoot '.\Builds\WindowsClient' `
  -Address 'HOST_IP:7777'
```

## Run From Repository Layout

On the host, from the repository root:

```powershell
.\Scripts\Playtest\StartPackagedServer.ps1 -Port 7777
```

In a second host terminal:

```powershell
.\Scripts\Playtest\StartPackagedClient.ps1 -Address '127.0.0.1:7777'
```

On the second machine, from a directory containing `WindowsClient` and the copied launcher:

```powershell
.\StartPackagedClient.ps1 -BuildRoot '.\WindowsClient' -Address 'HOST_IP:7777'
```

The scripts create isolated evidence under `Saved/Logs/DirectIPServer/<timestamp>` and
`Saved/Logs/DirectIPClient/<timestamp>`. Preserve both directories.

For the reconnect check on the remote machine, stop only its recorded client and relaunch with the
same address within 60 seconds:

```powershell
.\Scripts\Playtest\StopPackagedClient.ps1
.\Scripts\Playtest\StartPackagedClient.ps1 -BuildRoot '.\Builds\WindowsClient' `
  -Address 'HOST_IP:7777'
```

Copy the remote initial and reconnect evidence directories back to the host. After stopping the
server, review the three-client transport and reconnect logs together:

```powershell
.\Scripts\Playtest\ReviewDirectIPPlaytest.ps1 `
  -ServerDirectory 'SERVER_EVIDENCE' `
  -HostClientDirectory 'HOST_CLIENT_EVIDENCE' `
  -RemoteClientDirectories 'REMOTE_INITIAL_EVIDENCE','REMOTE_RECONNECT_EVIDENCE'
```

The automated reviewer expects three server joins: host client, initial remote client, and remote
reconnect. It does not judge the visible lobby, movement, ability, or cleanup checks.

## Acceptance

- [ ] `DI-01` The remote client reaches the lobby without `OutdatedClient`, timeout, travel, or network
  failure.
- [ ] `DI-02` The server records two initial joins and one remote reconnect; every client log records
  a welcome, and the server records successful restoration.
- [ ] `DI-03` The remote player changes team/slot, selects a hunter, readies, and reaches Playing.
- [ ] `DI-04` Host and remote player movement agree; each fires LMB and one persistent or movement ability
  while the other observes the same result and cleanup.
- [ ] `DI-05` Terminate the remote client during Playing, reconnect within 60 seconds with the same client
  config, and confirm team, slot, hunter, transform, vitals, and inventory restore.
- [ ] `DI-06` No server, host-client, initial-remote, or reconnect-remote log contains a critical
  error or GameplayCue overflow. Record route type, host IP,
  machine names, package timestamp, visible result, and all four evidence directories in the
  status tracker.

If LAN/VPN succeeds but public routing fails, use LAN/VPN for the July 31 playtest and record the
public-route limitation. A routing failure is not a reason to add matchmaking before the deadline.
