# Local Networking Smoke Tests

Use these scripts before asking other people to connect.

1. Start a local dedicated server:

   ```powershell
   .\Scripts\Networking\StartLocalDedicatedServer.ps1
   ```

2. Start local clients:

   ```powershell
   .\Scripts\Networking\StartLocalClients.ps1 -ClientCount 3
   ```

3. In each client, pick a hunter and press ready. The server should move through:
   `HunterSelect -> Countdown -> Dropping -> Playing -> PostMatch -> HunterSelect`.

For packet-loss and lag testing, use Unreal net emulation commands in each client console, for example:

```text
Net PktLag=100
Net PktLoss=2
```

## Headless Dedicated Handshake

Build both Game and Server targets with the same source-engine installation, then run:

```powershell
& 'C:\UnrealEngine-5.7.4-release\Engine\Build\BatchFiles\Build.bat' Breachborne Win64 Development -Project="$PWD\Breachborne.uproject" -WaitMutex
& 'C:\UnrealEngine-5.7.4-release\Engine\Build\BatchFiles\Build.bat' BreachborneServer Win64 Development -Project="$PWD\Breachborne.uproject" -WaitMutex
.\Scripts\Networking\TestHeadlessDedicatedHandshake.ps1 -ClientCount 2
```

The script starts hidden null-RHI clients, captures separate logs, validates `Welcomed by
server` and `Join succeeded`, and terminates only the processes it launched. Evidence is written
under `Saved/Logs/DedicatedHandshake/<timestamp>`.

Do not mix Launcher-engine clients with the source-engine dedicated server. Unreal rejects that
combination as `OutdatedClient` because the builds have different network version hashes.
