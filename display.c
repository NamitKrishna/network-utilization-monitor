#define _POSIX_C_SOURCE 200809L
#include "network_monitor.h"

#define CP_HEADER 1
#define CP_RX     2
#define CP_TX     3
#define CP_LABEL  4
#define CP_BORDER 5
#define CP_WARN   6
#define CP_TITLE  7
#define CP_DIM    8

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

static void draw_bar(int row, int col, int bar_width,
                     double value, double max_val, int cp)
{
    int filled = 0;
    int i;
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
    int i;
    for (i = 0; i < HISTORY_SIZE; i++)
        if (history[i] > max_val)
            max_val = history[i];

    int col;
    for (col = 0; col < width && col < HISTORY_SIZE; col++) {
        int idx = (next_idx + col) % HISTORY_SIZE;
        double ratio = history[idx] / max_val;
        int filled_rows = (int)(ratio * height);
        int row;
        for (row = 0; row < height; row++) {
            int screen_row = start_row + (height - 1 - row);
            char ch;
            if (row < filled_rows - 1)
                ch = '|';
            else if (row == filled_rows - 1)
                ch = ':';
            else
                ch = ' ';
            mvaddch(screen_row, start_col + col, ch);
        }
    }
}

void draw_dashboard(BandwidthInfo bw[], int count, int tick,
                    double uptime_sec)
{
    int rows, cols, c, i;
    getmaxyx(stdscr, rows, cols);
    erase();

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    for (c = 0; c < cols; c++)
        mvaddch(0, c, ' ');
    mvprintw(0, 2, "Network Utilization Monitor");
    mvprintw(0, cols - 26, "uptime %02d:%02d  tick %-4d",
             (int)(uptime_sec / 60), (int)uptime_sec % 60, tick);
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    attron(COLOR_PAIR(CP_LABEL) | A_BOLD);
    mvprintw(2, 2,  "Interface");
    mvprintw(2, 14, "RX KB/s");
    mvprintw(2, 24, "TX KB/s");
    mvprintw(2, 34, "RX bar");
    mvprintw(2, 56, "TX bar");
    mvprintw(2, 78, "RX MB");
    mvprintw(2, 86, "TX MB");
    attroff(COLOR_PAIR(CP_LABEL) | A_BOLD);

    attron(COLOR_PAIR(CP_BORDER));
    for (c = 0; c < cols; c++)
        mvaddch(3, c, '-');
    attroff(COLOR_PAIR(CP_BORDER));

    double max_kbps = 1.0;
    for (i = 0; i < count; i++) {
        if (bw[i].rx_kbps > max_kbps) max_kbps = bw[i].rx_kbps;
        if (bw[i].tx_kbps > max_kbps) max_kbps = bw[i].tx_kbps;
    }

    int row = 4;
    for (i = 0; i < count && row < rows - 16; i++) {
        BandwidthInfo *b = &bw[i];

        attron(COLOR_PAIR(CP_LABEL));
        mvprintw(row, 2, "%-11s", b->name);
        attroff(COLOR_PAIR(CP_LABEL));

        if (b->rx_kbps > 1024.0)
            attron(COLOR_PAIR(CP_WARN) | A_BOLD);
        else
            attron(COLOR_PAIR(CP_TITLE));
        mvprintw(row, 14, "%7.2f", b->rx_kbps);
        attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
        attroff(COLOR_PAIR(CP_TITLE));

        if (b->tx_kbps > 1024.0)
            attron(COLOR_PAIR(CP_WARN) | A_BOLD);
        else
            attron(COLOR_PAIR(CP_TITLE));
        mvprintw(row, 24, "%7.2f", b->tx_kbps);
        attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
        attroff(COLOR_PAIR(CP_TITLE));

        draw_bar(row, 34, 20, b->rx_kbps, max_kbps, CP_RX);
        draw_bar(row, 56, 20, b->tx_kbps, max_kbps, CP_TX);

        mvprintw(row, 78, "%7.3f", b->rx_mb_total);
        mvprintw(row, 86, "%7.3f", b->tx_mb_total);

        row++;
    }

    int graph_top = row + 2;
    int graph_h   = 8;
    int graph_w   = HISTORY_SIZE;

    if (graph_top + graph_h + 4 >= rows)
        goto footer;

    attron(COLOR_PAIR(CP_BORDER) | A_BOLD);
    mvprintw(graph_top - 1, 2, "[ RX KB/s - last %d seconds ]", HISTORY_SIZE);
    attroff(COLOR_PAIR(CP_BORDER) | A_BOLD);

    attron(COLOR_PAIR(CP_BORDER));
    int r;
    for (r = graph_top; r < graph_top + graph_h; r++)
        mvaddch(r, 1, '|');
    for (c = 2; c < 2 + graph_w + 1; c++)
        mvaddch(graph_top + graph_h, c, '-');
    attroff(COLOR_PAIR(CP_BORDER));

    if (count > 0) {
        attron(COLOR_PAIR(CP_RX));
        draw_graph(bw[0].rx_history, bw[0].history_idx,
                   graph_top, 2, graph_h, graph_w);
        attroff(COLOR_PAIR(CP_RX));
    }

    attron(COLOR_PAIR(CP_RX));
    mvprintw(graph_top + graph_h + 1, 2, "RX (green)");
    attroff(COLOR_PAIR(CP_RX));
    attron(COLOR_PAIR(CP_TX));
    mvprintw(graph_top + graph_h + 1, 16, "TX (blue)");
    attroff(COLOR_PAIR(CP_TX));

footer:
    attron(COLOR_PAIR(CP_HEADER));
    for (c = 0; c < cols; c++)
        mvaddch(rows - 1, c, ' ');
    mvprintw(rows - 1, 2,
             "q=quit  r=reset  interval:%ds  /proc/net/dev",
             REFRESH_SEC);
    attroff(COLOR_PAIR(CP_HEADER));

    refresh();
}
