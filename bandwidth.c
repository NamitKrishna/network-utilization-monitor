#define _POSIX_C_SOURCE 200809L
#include "network_monitor.h"

void compute_bandwidth(InterfaceStats *prev, InterfaceStats *curr,
                       BandwidthInfo  *bw,  double elapsed_sec)
{
    if (elapsed_sec <= 0.0) elapsed_sec = 1.0;

    unsigned long long rx_diff = (curr->rx_bytes >= prev->rx_bytes)
                                 ? curr->rx_bytes - prev->rx_bytes : 0;
    unsigned long long tx_diff = (curr->tx_bytes >= prev->tx_bytes)
                                 ? curr->tx_bytes - prev->tx_bytes : 0;

    bw->rx_kbps = (double)rx_diff / elapsed_sec / 1024.0;
    bw->tx_kbps = (double)tx_diff / elapsed_sec / 1024.0;

    /* track peaks */
    if (bw->rx_kbps > bw->peak_rx_kbps) bw->peak_rx_kbps = bw->rx_kbps;
    if (bw->tx_kbps > bw->peak_tx_kbps) bw->peak_tx_kbps = bw->tx_kbps;

    bw->rx_mb_total += (double)rx_diff / (1024.0 * 1024.0);
    bw->tx_mb_total += (double)tx_diff / (1024.0 * 1024.0);

    bw->rx_history[bw->history_idx] = bw->rx_kbps;
    bw->tx_history[bw->history_idx] = bw->tx_kbps;
    bw->history_idx = (bw->history_idx + 1) % HISTORY_SIZE;

    bw->active = (curr->rx_bytes > 0 || curr->tx_bytes > 0);

    snprintf(bw->name, IFACE_NAME_LEN, "%.31s", curr->name);
}
