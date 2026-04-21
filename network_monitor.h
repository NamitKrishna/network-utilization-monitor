#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>
#include <signal.h>
#include <math.h>

#define MAX_INTERFACES   16
#define IFACE_NAME_LEN   32
#define HISTORY_SIZE     50
#define PROC_NET_DEV     "/proc/net/dev"
#define REFRESH_SEC      1

typedef struct {
    char               name[IFACE_NAME_LEN];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_packets;
    unsigned long long tx_packets;
    unsigned long long rx_errors;
    unsigned long long tx_errors;
    int                valid;
} InterfaceStats;

typedef struct {
    char   name[IFACE_NAME_LEN];
    double rx_kbps;
    double tx_kbps;
    double peak_rx_kbps;
    double peak_tx_kbps;
    double rx_mb_total;
    double tx_mb_total;
    double rx_history[HISTORY_SIZE];
    double tx_history[HISTORY_SIZE];
    int    history_idx;
    int    active;
} BandwidthInfo;

int  read_proc_net_dev(InterfaceStats stats[], int *count);
void compute_bandwidth(InterfaceStats *prev, InterfaceStats *curr,
                       BandwidthInfo  *bw,  double elapsed_sec);
void init_display(void);
void draw_dashboard(BandwidthInfo bw[], int count, int tick, double uptime_sec);
void cleanup_display(void);

#endif
