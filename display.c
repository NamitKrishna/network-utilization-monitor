#define _POSIX_C_SOURCE 200809L
#include "network_monitor.h"

#define CP_HEADER  1
#define CP_RX      2
#define CP_TX      3
#define CP_LABEL   4
#define CP_BORDER  5
#define CP_WARN    6
#define CP_TITLE   7
#define CP_DIM     8
#define CP_SUCCESS 9
#define CP_PEAK    10

void init_display(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(0);
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(CP_HEADER,  COLOR_BLACK,  COLOR_CYAN);
        init_pair(CP_RX,      COLOR_BLACK,  COLOR_GREEN);
        init_pair(CP_TX,      COLOR_BLACK,  COLOR_BLUE);
        init_pair(CP_LABEL,   COLOR_CYAN,   -1);
        init_pair(CP_BORDER,  COLOR_YELLOW, -1);
        init_pair(CP_WARN,    COLOR_RED,    -1);
        init_pair(CP_TITLE,   COLOR_WHITE,  -1);
        init_pair(CP_DIM,     COLOR_WHITE,  -1);
        init_pair(CP_SUCCESS, COLOR_GREEN,  -1);
        init_pair(CP_PEAK,    COLOR_MAGENTA,-1);
    }
}

void cleanup_display(void)
{
    endwin();
}

static void draw_bar(int row, int col, int bar_width,
                     double value, double max_val, int cp)
{
    int filled = 0, i;
    if (max_val > 0.0)
        filled = (int)((value / max_val) * bar_width);
    if (filled > bar_width) filled = bar_width;

    attron(COLOR_PAIR(cp));
    for (i = 0; i < filled; i++)
        mvaddch(row, col + i, ' ');
    attroff(COLOR_PAIR(cp));

    attron(COLOR_PAIR(CP_DIM));
    for (i = filled; i < bar_width; i++)
        mvaddch(row, col + i, '-');
    attroff(COLOR_PAIR(CP_DIM));
}

static void draw_graph(double history[], int next_idx,
                       int start_row, int start_col,
                       int height, int width)
{
    double max_val = 1.0;
    int i, col;
    for (i = 0; i < HISTORY_SIZE; i++)
        if (history[i] > max_val)
            max_val = history[i];

    for (col = 0; col < width && col < HISTORY_SIZE; col++) {
        int idx = (next_idx + col) % HISTORY_SIZE;
        double ratio = history[idx] / max_val;
        int filled_rows = (int)(ratio * height);
        int row;
        for (row = 0; row < height; row++) {
            int screen_row = start_row + (height - 1 - row);
            char ch;
            if (row < filled_rows - 1)      ch = '|';
            else if (row == filled_rows - 1) ch = ':';
            else                             ch = ' ';
            mvaddch(screen_row, start_col + col, ch);
        }
    }
}

/* Returns a speed label based on KB/s */
static const char *speed_label(double kbps)
{
    if (kbps < 10.0)   return "  IDLE  ";
    if (kbps < 100.0)  return "  SLOW  ";
    if (kbps < 1024.0) return " NORMAL ";
    if (kbps < 5120.0) return "  FAST  ";
    return "BLAZING!";
}

static int speed_color(double kbps)
{
    if (kbps < 10.0)   return CP_DIM;
    if (kbps < 100.0)  return CP_LABEL;
    if (kbps < 1024.0) return CP_SUCCESS;
    if (kbps < 5120.0) return CP_BORDER;
    return CP_WARN;
}

void draw_dashboard(BandwidthInfo bw[], int count, int tick,
                    double uptime_sec)
{
    int rows, cols, c, i;
    getmaxyx(stdscr, rows, cols);
    erase();

    /* ── Header ── */
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    for (c = 0; c < cols; c++) mvaddch(0, c, ' ');
    mvprintw(0, 2, "  Network Utilization Monitor  |  CN Project  |  /proc/net/dev");
    mvprintw(0, cols - 26, "uptime %02d:%02d  tick %-4d",
             (int)(uptime_sec / 60), (int)uptime_sec % 60, tick);
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    /* ── Sub header ── */
    attron(COLOR_PAIR(CP_BORDER));
    mvprintw(1, 2, "Monitoring live bandwidth | RX = Download  TX = Upload | Press q=quit  r=reset peaks");
    attroff(COLOR_PAIR(CP_BORDER));

    /* ── Column headings ── */
    attron(COLOR_PAIR(CP_LABEL) | A_BOLD);
    mvprintw(3, 2,  "Interface");
    mvprintw(3, 14, "RX KB/s");
    mvprintw(3, 24, "TX KB/s");
    mvprintw(3, 34, "RX bar");
    mvprintw(3, 56, "TX bar");
    mvprintw(3, 78, "Peak RX");
    mvprintw(3, 88, "RX MB");
    mvprintw(3, 96, "TX MB");
    mvprintw(3, 104, "Status");
    attroff(COLOR_PAIR(CP_LABEL) | A_BOLD);

    attron(COLOR_PAIR(CP_BORDER));
    for (c = 0; c < cols; c++) mvaddch(4, c, '-');
    attroff(COLOR_PAIR(CP_BORDER));

    /* ── Find max for scaling ── */
    double max_kbps = 1.0;
    for (i = 0; i < count; i++) {
        if (bw[i].rx_kbps > max_kbps) max_kbps = bw[i].rx_kbps;
        if (bw[i].tx_kbps > max_kbps) max_kbps = bw[i].tx_kbps;
    }

    /* ── Interface rows ── */
    int row = 5;
    for (i = 0; i < count && row < rows - 16; i++) {
        BandwidthInfo *b = &bw[i];

        /* update peak */
        if (b->rx_kbps > b->peak_rx_kbps)
            b->peak_rx_kbps = b->rx_kbps;

        attron(COLOR_PAIR(CP_LABEL));
        mvprintw(row, 2, "%-11s", b->name);
        attroff(COLOR_PAIR(CP_LABEL));

        /* RX speed */
        attron(COLOR_PAIR(speed_color(b->rx_kbps)));
        mvprintw(row, 14, "%7.2f", b->rx_kbps);
        attroff(COLOR_PAIR(speed_color(b->rx_kbps)));

        /* TX speed */
        attron(COLOR_PAIR(speed_color(b->tx_kbps)));
        mvprintw(row, 24, "%7.2f", b->tx_kbps);
        attroff(COLOR_PAIR(speed_color(b->tx_kbps)));

        /* bars */
        draw_bar(row, 34, 20, b->rx_kbps, max_kbps, CP_RX);
        draw_bar(row, 56, 20, b->tx_kbps, max_kbps, CP_TX);

        /* peak RX */
        attron(COLOR_PAIR(CP_PEAK));
        mvprintw(row, 78, "%7.2f", b->peak_rx_kbps);
        attroff(COLOR_PAIR(CP_PEAK));

        /* totals */
        mvprintw(row, 88, "%5.2f", b->rx_mb_total);
        mvprintw(row, 96, "%5.2f", b->tx_mb_total);

        /* status label */
        attron(COLOR_PAIR(speed_color(b->rx_kbps)) | A_BOLD);
        mvprintw(row, 104, "%s", speed_label(b->rx_kbps));
        attroff(COLOR_PAIR(speed_color(b->rx_kbps)) | A_BOLD);

        row++;
    }

    /* ── Separator ── */
    attron(COLOR_PAIR(CP_BORDER));
    for (c = 0; c < cols; c++) mvaddch(row + 1, c, '-');
    attroff(COLOR_PAIR(CP_BORDER));

    /* ── Graph ── */
    int graph_top = row + 3;
    int graph_h   = 8;
    int graph_w   = HISTORY_SIZE;

    if (graph_top + graph_h + 5 >= rows) goto footer;

    attron(COLOR_PAIR(CP_BORDER) | A_BOLD);
    mvprintw(graph_top - 1, 2, "[ RX Bandwidth Graph - last %d seconds ]", HISTORY_SIZE);
    attroff(COLOR_PAIR(CP_BORDER) | A_BOLD);

    attron(COLOR_PAIR(CP_BORDER));
    int r;
    for (r = graph_top; r < graph_top + graph_h; r++)
        mvaddch(r, 1, '|');
    for (c = 2; c < 2 + graph_w + 1; c++)
        mvaddch(graph_top + graph_h, c, '-');
    mvaddch(graph_top + graph_h, 1, '+');
    attroff(COLOR_PAIR(CP_BORDER));

    if (count > 0) {
        attron(COLOR_PAIR(CP_RX));
        draw_graph(bw[0].rx_history, bw[0].history_idx,
                   graph_top, 2, graph_h, graph_w);
        attroff(COLOR_PAIR(CP_RX));
    }

    /* ── Graph legend ── */
    attron(COLOR_PAIR(CP_RX) | A_BOLD);
    mvprintw(graph_top + graph_h + 1, 2, "RX Download (green)");
    attroff(COLOR_PAIR(CP_RX) | A_BOLD);

    attron(COLOR_PAIR(CP_TX) | A_BOLD);
    mvprintw(graph_top + graph_h + 1, 24, "TX Upload (blue)");
    attroff(COLOR_PAIR(CP_TX) | A_BOLD);

    attron(COLOR_PAIR(CP_PEAK));
    mvprintw(graph_top + graph_h + 1, 44, "Magenta = Peak speed");
    attroff(COLOR_PAIR(CP_PEAK));

    /* ── Alert if any interface blazing ── */
    for (i = 0; i < count; i++) {
        if (bw[i].rx_kbps > 5120.0) {
            attron(COLOR_PAIR(CP_WARN) | A_BOLD | A_BLINK);
            mvprintw(graph_top + graph_h + 2, 2,
                     "!! HIGH TRAFFIC ALERT on %s: %.2f KB/s !!",
                     bw[i].name, bw[i].rx_kbps);
            attroff(COLOR_PAIR(CP_WARN) | A_BOLD | A_BLINK);
        }
    }

footer:
    attron(COLOR_PAIR(CP_HEADER));
    for (c = 0; c < cols; c++) mvaddch(rows - 1, c, ' ');
    mvprintw(rows - 1, 2,
             "q=quit  r=reset peaks  interval:%ds  source:/proc/net/dev  "
             "Status: IDLE<10 SLOW<100 NORMAL<1024 FAST<5120 BLAZING",
             REFRESH_SEC);
    attroff(COLOR_PAIR(CP_HEADER));

    refresh();
}
