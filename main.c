#define _POSIX_C_SOURCE 200809L
#include "network_monitor.h"

static volatile int running = 1;
static void handle_signal(int sig) { (void)sig; running = 0; }

int main(void)
{
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    InterfaceStats prev[MAX_INTERFACES];
    InterfaceStats curr[MAX_INTERFACES];
    BandwidthInfo  bw[MAX_INTERFACES];
    int prev_count = 0, curr_count = 0;

    memset(prev, 0, sizeof(prev));
    memset(curr, 0, sizeof(curr));
    memset(bw,   0, sizeof(bw));

    if (read_proc_net_dev(prev, &prev_count) < 0) {
        fprintf(stderr, "Cannot open %s\n", PROC_NET_DEV);
        return 1;
    }

    init_display();

    struct timespec t_start, t_prev, t_curr;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    t_prev = t_start;

    int tick = 0;

    while (running) {
        sleep(REFRESH_SEC);

        clock_gettime(CLOCK_MONOTONIC, &t_curr);
        double elapsed = (double)(t_curr.tv_sec  - t_prev.tv_sec)
                       + (double)(t_curr.tv_nsec - t_prev.tv_nsec) / 1e9;
        double uptime  = (double)(t_curr.tv_sec  - t_start.tv_sec)
                       + (double)(t_curr.tv_nsec - t_start.tv_nsec) / 1e9;
        t_prev = t_curr;

        if (read_proc_net_dev(curr, &curr_count) < 0) break;

        int display_count = 0;
        int i, j;
        for (i = 0; i < curr_count; i++) {
            for (j = 0; j < prev_count; j++) {
                if (strcmp(curr[i].name, prev[j].name) == 0) {
                    compute_bandwidth(&prev[j], &curr[i],
                                      &bw[display_count++], elapsed);
                    break;
                }
            }
        }

        tick++;
        draw_dashboard(bw, display_count, tick, uptime);

        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
        if (ch == 'r' || ch == 'R') {
            for (i = 0; i < display_count; i++) {
                bw[i].rx_mb_total  = 0.0;
                bw[i].tx_mb_total  = 0.0;
                bw[i].peak_rx_kbps = 0.0;
                bw[i].peak_tx_kbps = 0.0;
            }
        }

        memcpy(prev, curr, sizeof(curr));
        prev_count = curr_count;
    }

    cleanup_display();
    printf("\nNetwork Monitor stopped. Goodbye!\n");
    return 0;
}
