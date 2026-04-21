#define _POSIX_C_SOURCE 200809L
#include "network_monitor.h"

int read_proc_net_dev(InterfaceStats stats[], int *count)
{
    FILE *fp = fopen(PROC_NET_DEV, "r");
    if (!fp) { perror("fopen /proc/net/dev"); return -1; }

    char line[2048];
    *count = 0;

    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return -1; }
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return -1; }

    while (fgets(line, sizeof(line), fp) && *count < MAX_INTERFACES) {
        InterfaceStats *s = &stats[*count];
        memset(s, 0, sizeof(*s));

        char *colon = strchr(line, ':');
        if (!colon) continue;

        *colon = '\0';
        char *name = line;
        while (*name == ' ') name++;
        snprintf(s->name, IFACE_NAME_LEN, "%.31s", name);

        unsigned long long dummy;
        int n = sscanf(colon + 1,
            "%llu %llu %llu %llu %llu %llu %llu %llu"
            " %llu %llu %llu %llu %llu %llu %llu %llu",
            &s->rx_bytes,   &s->rx_packets,
            &s->rx_errors,  &dummy,
            &dummy,         &dummy,
            &dummy,         &dummy,
            &s->tx_bytes,   &s->tx_packets,
            &s->tx_errors,  &dummy,
            &dummy,         &dummy,
            &dummy,         &dummy);

        if (n >= 10) {
            s->valid = 1;
            (*count)++;
        }
    }
    fclose(fp);
    return 0;
}
