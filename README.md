# Network Utilization Monitor

> Computer Networks Subject Project  
> Platform: Linux (Kali) | Language: C | UI: ncurses

---

## What it does

| Requirement | Implementation |
|---|---|
| Collect byte counters | Reads `/proc/net/dev` every second |
| Estimate bandwidth | `(bytes_now − bytes_before) ÷ elapsed_sec → KB/s` |
| Display utilization | Live ncurses dashboard with colour bars + ASCII graph |
| Update periodically | Configurable `REFRESH_SEC` interval (default 1 s) |

---

## Project structure

```
network_monitor/
├── main.c            # Program entry, main loop, signal handling
├── collector.c       # Reads /proc/net/dev → raw byte counters
├── bandwidth.c       # Diffs counters → KB/s, rolling history
├── display.c         # ncurses dashboard (bars + graph + table)
├── network_monitor.h # Shared types and prototypes
└── Makefile
```

---

## Build & run on Kali Linux

```bash
# Install ncurses if needed
sudo apt install libncurses5-dev libncursesw5-dev

# Build
make

# Run (needs permission to read /proc/net/dev — usually fine without sudo)
./network_monitor

# Press q to quit, r to reset totals
```

---

## Key concepts demonstrated

- **`/proc/net/dev`** — Linux kernel virtual filesystem exposing live network counters
- **Byte counter differencing** — bandwidth = Δbytes / Δtime
- **Circular buffer** — stores 50-sample rolling history for the graph
- **ncurses** — portable terminal UI library
- **Signal handling** — `SIGINT`/`SIGTERM` for clean exit

---

## Controls

| Key | Action |
|---|---|
| `q` | Quit |
| `r` | Reset MB totals |

---

## Sample output

```
 Network Utilization Monitor          uptime 00:32  tick 32

Interface   RX KB/s  TX KB/s  RX bar              TX bar           RX MB tot  TX MB tot
---------------------------------------------------------------------------
eth0          42.31     1.20  ████████░░░░░░░░    ██░░░░░░░░░░░░     1.234      0.038
lo             0.00     0.00  ░░░░░░░░░░░░░░░░    ░░░░░░░░░░░░░░     0.000      0.000

[ RX KB/s — last 50 seconds ]
|    |
|   |||
|  |||||
| |||||||  |
|||||||||||||
--------------------------------

 q = quit   r = reset totals   interval: 1s   /proc/net/dev
```
