/*
 * display.c
 * ncurses dashboard:
 *   - header with uptime and tick counter
 *   - per-interface bandwidth bars (RX green, TX blue)
 *   - rolling ASCII graph (last 50 seconds)
 *   - totals table
 *   - footer with keybinds
 */
#include "network_monitor.h"

/* colour pair IDs */
#define CP_HEADER   1
#define CP_RX       2
#define CP_TX       3
#define CP_LABEL    4
#define CP_BORDER   5
#define CP_WARN     6
#define CP_TITLE    7
#define CP_DIM      8

void init_display(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(0);   /* non-blocking getch */

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(CP_HEADER, COLOR_BLACK,  COLOR_CYAN);
        init_pair(CP_RX,     COLOR_BLACK,  COLOR_GREEN);
        init_pair(CP_TX,     COLOR_BLACK,  COLOR_BLUE);
        init_pair(CP_LABEL,  COLOR_CYAN,   -1);
        init_pair(CP_BORDER, COLOR_YELLOW, -1);
        init_pair(CP_WARN,   COLOR_RED,    -1);
        init_pair(CP_TITLE,  COLOR_WHITE,  -1);
        init_pair(CP_DIM,    COLOR_WHITE,  -1);
    }
}

void cleanup_display(void)
{
    endwin();
}

/* Draw a filled progress bar inside the current window */
static void draw_bar(int row, int col, int bar_width,
                     double value, double max_val, int cp)
{
    int filled = 0;
    if (max_val > 0.0)
        filled = (int)((value / max_val) * bar_width);
    if (filled > bar_width) filled = bar_width;

    attron(COLOR_PAIR(cp));
    for (int i = 0; i < filled; i++)
        mvaddch(row, col + i, ' ');
    attroff(COLOR_PAIR(cp));

    /* empty portion */
    attron(COLOR_PAIR(CP_DIM));
    for (int i = filled; i < bar_width; i++)
        mvaddch(row, col + i, '-');
    attroff(COLOR_PAIR(CP_DIM));
}

/* Draw a small ASCII sparkline graph */
static void draw_graph(double history[], int next_idx,
                       int start_row, int start_col,
                       int height, int width)
{
    /* find the max value for scaling */
    double max_val = 1.0;
    for (int i = 0; i < HISTORY_SIZE; i++)
        if (history[i] > max_val) max_val = history[i];

    /* graph characters from low to high density */
    const char *blocks = " .:!|";
    int nb = 4;

    for (int col = 0; col < width && col < HISTORY_SIZE; col++) {
        /* walk history oldest-first */
        int idx = (next_idx + col) % HISTORY_SIZE;
        double val = history[idx];
        double ratio = val / max_val;

        /* fill vertically */
        int filled_rows = (int)(ratio * height);
        for (int row = 0; row < height; row++) {
            int screen_row = start_row + (height - 1 - row);
            char ch;
            if (row < filled_rows - 1)
                ch = '|';
            else if (row == filled_rows - 1)
                ch = blocks[(int)(ratio * nb) % (nb + 1)];
            else
                ch = ' ';
            mvaddch(screen_row, start_col + col, ch);
        }
    }
}

void draw_dashboard(BandwidthInfo bw[], int count, int tick,
                    double uptime_sec)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    erase();

    /* ── Header bar ─────────────────────────────────── */
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    for (int c = 0; c < cols; c++) mvaddch(0, c, ' ');
    int mins = (int)(uptime_sec / 60);
    int secs = (int)uptime_sec % 60;
    mvprintw(0, 2, " Network Utilization Monitor ");
    mvprintw(0, cols - 28, "uptime %02d:%02d  tick %-4d", mins, secs, tick);
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    /* ── Column headings ─────────────────────────────── */
    attron(COLOR_PAIR(CP_LABEL) | A_BOLD);
    mvprintw(2, 2,  "Interface");
    mvprintw(2, 14, "RX KB/s");
    mvprintw(2, 24, "TX KB/s");
    mvprintw(2, 34, "RX bar");
    int bar_w = cols - 70;
    if (bar_w < 10) bar_w = 10;
    mvprintw(2, 34 + bar_w + 2, "TX bar");
    mvprintw(2, cols - 22, "RX MB tot");
    mvprintw(2, cols - 11, "TX MB tot");
    attroff(COLOR_PAIR(CP_LABEL) | A_BOLD);

    /* separator */
    attron(COLOR_PAIR(CP_BORDER));
    for (int c = 0; c < cols; c++) mvaddch(3, c, '-');
    attroff(COLOR_PAIR(CP_BORDER));

    /* ── Per-interface rows ───────────────────────────── */
    /* find max kbps across all ifaces for bar scaling */
    double max_kbps = 1.0;
    for (int i = 0; i < count; i++) {
        if (bw[i].rx_kbps > max_kbps) max_kbps = bw[i].rx_kbps;
        if (bw[i].tx_kbps > max_kbps) max_kbps = bw[i].tx_kbps;
    }

    int row = 4;
    for (int i = 0; i < count && row < rows - 14; i++) {
        BandwidthInfo *b = &bw[i];

        /* dim inactive interfaces */
        if (!b->active) attron(A_DIM);

        attron(COLOR_PAIR(CP_LABEL));
        mvprintw(row, 2, "%-11s", b->name);
        attroff(COLOR_PAIR(CP_LABEL));

        /* RX speed – warn if > 1 MB/s */
        if (b->rx_kbps > 1024.0) attron(COLOR_PAIR(CP_WARN) | A_BOLD);
        else                      attron(COLOR_PAIR(CP_TITLE));
        mvprintw(row, 14, "%8.2f", b->rx_kbps);
        attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
        attroff(COLOR_PAIR(CP_TITLE));

        if (b->tx_kbps > 1024.0) attron(COLOR_PAIR(CP_WARN) | A_BOLD);
        else                      attron(COLOR_PAIR(CP_TITLE));
        mvprintw(row, 24, "%8.2f", b->tx_kbps);
        attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
        attroff(COLOR_PAIR(CP_TITLE));

        /* RX bar */
        draw_bar(row, 34, bar_w, b->rx_kbps, max_kbps, CP_RX);

        /* TX bar */
        draw_bar(row, 34 + bar_w + 2, bar_w, b->tx_kbps, max_kbps, CP_TX);

        /* totals */
        mvprintw(row, cols - 22, "%8.3f", b->rx_mb_total);
        mvprintw(row, cols - 11, "%8.3f", b->tx_mb_total);

        if (!b->active) attroff(A_DIM);
        row++;
    }

    /* ── Graph section ───────────────────────────────── */
    int graph_top = row + 2;
    int graph_h   = 8;
    int graph_w   = HISTORY_SIZE;
    if (graph_top + graph_h + 4 > rows) goto footer;

    attron(COLOR_PAIR(CP_BORDER) | A_BOLD);
    mvprintw(graph_top - 1, 2, "[ RX KB/s — last %d seconds ]", HISTORY_SIZE);
    attroff(COLOR_PAIR(CP_BORDER) | A_BOLD);

    /* draw graph border */
    for (int r = graph_top; r < graph_top + graph_h; r++) {
        attron(COLOR_PAIR(CP_BORDER));
        mvaddch(r, 1, '|');
        attroff(COLOR_PAIR(CP_BORDER));
    }
    attron(COLOR_PAIR(CP_BORDER));
    for (int c = 2; c < 2 + graph_w + 1; c++)
        mvaddch(graph_top + graph_h, c, '-');
    attroff(COLOR_PAIR(CP_BORDER));

    /* draw RX history of first active interface */
    attron(COLOR_PAIR(CP_RX));
    for (int i = 0; i < count; i++) {
        if (bw[i].active || i == 0) {
            draw_graph(bw[i].rx_history, bw[i].history_idx,
                       graph_top, 2, graph_h, graph_w);
            break;
        }
    }
    attroff(COLOR_PAIR(CP_RX));

    /* legend */
    attron(COLOR_PAIR(CP_RX));
    mvprintw(graph_top + graph_h + 1, 2, "  RX (green)");
    attroff(COLOR_PAIR(CP_RX));
    attron(COLOR_PAIR(CP_TX));
    mvprintw(graph_top + graph_h + 1, 20, "  TX (blue)");
    attroff(COLOR_PAIR(CP_TX));

footer:
    /* ── Footer ──────────────────────────────────────── */
    attron(COLOR_PAIR(CP_HEADER));
    for (int c = 0; c < cols; c++) mvaddch(rows - 1, c, ' ');
    mvprintw(rows - 1, 2, " q = quit   r = reset totals   "
                          "interval: %ds   /proc/net/dev", REFRESH_SEC);
    attroff(COLOR_PAIR(CP_HEADER));

    refresh();
}
