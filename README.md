# Network Utilization Monitor
Real-time network bandwidth monitor built in C for Linux.

## How it works

- Reads /proc/net/dev every second to collect byte counters
- Estimates bandwidth using: (bytes_now - bytes_before) / elapsed_sec
- Displays live ncurses terminal dashboard with colour bars
- Updates periodically using sleep(1) loop

## Project Structure

- main.c            Entry point, main loop, signal handling
- collector.c       Reads /proc/net/dev raw byte counters
- bandwidth.c       Computes KB/s and maintains rolling history
- display.c         ncurses live dashboard with bars and graph
- network_monitor.h Shared structs and function prototypes
- Makefile          One command build

## Build and Run

sudo apt install libncurses5-dev -y
make
./network_monitor

## Controls

q = quit
r = reset MB totals

## Platform

Kali Linux | Language: C | Subject: Computer Networks


