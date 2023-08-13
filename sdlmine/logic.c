
#include "logic.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "main.h"

static int won(void);

unsigned minefield_x = 9, minefield_y = 9, minefield_mines = 10;
char * minefield = NULL;
char * minefield_visible = NULL;
char * minefield_unlose_buf = NULL;

static void call_around(int i, void (*fn)(int, int, int), int n) {
    int x = i % minefield_x;
    int y = i / minefield_x;
    for (int dx = -1; dx <= 1; dx++)
    for (int dy = -1; dy <= 1; dy++) {
        int nx = x + dx;
        int ny = y + dy;
        int j = nx + ny * minefield_x;
        if (nx >= 0 && ny >= 0 && nx < minefield_x && ny < minefield_y && i != j)
            fn(i, j, n);
    }
}

static void add_inner(int i, int j, int _n) {
    if (minefield[j] == 9) minefield[i] += 1;
}

static void add_outer(int i, int j, int n) {
    if (minefield[j] < 9) minefield[j] += n;
}

void regenerate_minefield(unsigned x, unsigned y, unsigned mines) {
    srand(time(NULL));

    free(minefield);
    free(minefield_visible);
    free(minefield_unlose_buf);

    minefield = calloc(x * y, 1);
    minefield_visible = calloc(x * y, 1);
    minefield_unlose_buf = calloc(x * y, 1);

    for (unsigned i = 0; i < mines; i++) {
        unsigned mine_x = rand() % x;
        unsigned mine_y = rand() % y;
        if (minefield[mine_y * x + mine_x] == 9) {
            i--;
            continue;
        }
        minefield[mine_y * x + mine_x] = 9;
        call_around(mine_y * x + mine_x, add_outer, 1);
    }

    for (unsigned i = 0; i < x * y; i++) {
        minefield_visible[i] = VIS_COVERED;
    }

    mine_counter = mines;
    first_move = 1;
}

static void flood_uncover(int _i, int j, int _n) {
    if (minefield[j] == 9) return;
    if (minefield_visible[j] != VIS_COVERED) return;

    minefield_visible[j] = VIS_UNCOVERED;
    if (minefield[j] == 0) {
        call_around(j, flood_uncover, 0);
    }
}

int mine_counter = 0;

int toggle_flag(int x, int y) {
    int vis = minefield_visible[x + y * minefield_x];
    int min = minefield[x + y * minefield_x];
    if (vis == VIS_COVERED) {
        minefield_visible[x + y * minefield_x] = VIS_FLAG;
        mine_counter--;
        if (mine_counter < 0) mine_counter = 0;
    } else if (vis == VIS_FLAG) {
        minefield_visible[x + y * minefield_x] = VIS_QUESTION;
        mine_counter++;
        if (mine_counter > 999) mine_counter = 999;
    } else if (vis == VIS_QUESTION)
        minefield_visible[x + y * minefield_x] = VIS_COVERED;
    return won();
}

static int won(void) {
    int won = 1;
    for (int i = 0; i < minefield_x * minefield_y; i++) {
        if (minefield[i] == 9 && minefield_visible[i] != VIS_FLAG) {
            won = 0;
            break;
        }
        if (minefield[i] != 9 && minefield_visible[i] == VIS_COVERED) {
            won = 0;
            break;
        }
    }
    return won;
}

char first_move = 1;

int make_move(int x, int y) {
    int i = x + y * minefield_x;
    int vis = minefield_visible[i];
    int min = minefield[i];
    if (vis != VIS_COVERED) return 0;
    if (min == 9) {
        if (first_move) {
            int not_mines = 0;
            for (int j = 0; j < minefield_x * minefield_y; j++) {
                if(minefield[j] != 9) {
                    minefield_unlose_buf[not_mines] = j;
                    not_mines++;
                }
            }
            if (not_mines > 0) {
                int j = minefield_unlose_buf[rand() % not_mines];
                minefield[i] = 0;
                minefield[j] = 9;
                call_around(i, add_outer, -1);
                call_around(j, add_outer, 1);
                call_around(i, add_inner, 0);
                return make_move(x, y);
            }
        }
        for (int j = 0; j < minefield_x * minefield_y; j++) {
            if (minefield[j] == 9) minefield_visible[j] = VIS_OTHER_MINE;
            if (minefield_visible[j] == VIS_FLAG && minefield[j] != 9) minefield_visible[j] = VIS_WRONG_FLAG;
        }
        minefield_visible[i] = VIS_LOSING_MINE;
        return -1;
    } else {
        first_move = 0;
        flood_uncover(0, i, 0);
        return won();
    }
}

#include <stdio.h>

void load_settings() {
    FILE * in = fopen("sdlmine.dat", "r");
    if (in == NULL) {
        minefield_x = 9;
        minefield_y = 9;
        minefield_mines = 10;
        black_white = 0;
        sounds_enabled = 1;
        save_settings();
    } else {
        fscanf(in, "%d %d %d\n", &minefield_x, &minefield_y, &minefield_mines);
        fscanf(in, "%d %d\n", &black_white, &sounds_enabled);
        fclose(in);
    }
}

void save_settings() {
    FILE * out = fopen("sdlmine.dat", "w");
    fprintf(out, "%d %d %d\n", minefield_x, minefield_y, minefield_mines);
    fprintf(out, "%d %d\n", black_white, sounds_enabled);
    fflush(out);
    fclose(out);
}
