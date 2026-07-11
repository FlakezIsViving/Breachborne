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
