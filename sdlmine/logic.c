
#include "logic.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "main.h"

static int won(void);
static void call_around(int,  void (*)(int, int, int), int);
static void add_inner(int i, int j, int n);
static void add_outer(int i, int j, int n);

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

static void flood_uncover(int x, int y) {
    // We are guaranteed that (x,y) is not a mine.
    // Recurse to every tile around (x,y) as long as it's
    //   1) minefield_visible == VIS_COVERED 2) minefield == 0

    minefield_visible[y * minefield_x + x] = VIS_UNCOVERED;

    if (minefield[y * minefield_x + x] == 0) {
        if (x > 0 && y > 0 && minefield_visible[(y - 1) * minefield_x + (x - 1)] == VIS_COVERED &&
            minefield[(y - 1) * minefield_x + (x - 1)] <= 8) {
            minefield_visible[(y - 1) * minefield_x + (x - 1)] = VIS_UNCOVERED;
            flood_uncover(x - 1, y - 1);
        }

        if (y > 0 && minefield_visible[(y - 1) * minefield_x + x] == VIS_COVERED &&
            minefield[(y - 1) * minefield_x + x] <= 8) {
            minefield_visible[(y - 1) * minefield_x + x] = VIS_UNCOVERED;
            flood_uncover(x, y - 1);
        }

        if (x < minefield_x - 1 && y > 0 && minefield_visible[(y - 1) * minefield_x + (x + 1)] == VIS_COVERED &&
            minefield[(y - 1) * minefield_x + (x + 1)] <= 8) {
            minefield_visible[(y - 1) * minefield_x + (x + 1)] = VIS_UNCOVERED;
            flood_uncover(x + 1, y - 1);
        }

        if (x > 0 && minefield_visible[y * minefield_x + (x - 1)] == VIS_COVERED &&
            minefield[y * minefield_x + (x - 1)] <= 8) {
            minefield_visible[y * minefield_x + (x - 1)] = VIS_UNCOVERED;
            flood_uncover(x - 1, y);
        }

        if (x < minefield_x - 1 && y < minefield_y - 1 &&
            minefield_visible[(y + 1) * minefield_x + (x + 1)] == VIS_COVERED &&
            minefield[(y + 1) * minefield_x + (x + 1)] <= 8) {
            minefield_visible[(y + 1) * minefield_x + (x + 1)] = VIS_UNCOVERED;
            flood_uncover(x + 1, y + 1);
        }

        if (x > 0 && y < minefield_y - 1 && minefield_visible[(y + 1) * minefield_x + (x - 1)] == VIS_COVERED &&
            minefield[(y + 1) * minefield_x + (x - 1)] <= 8) {
            minefield_visible[(y + 1) * minefield_x + (x - 1)] = VIS_UNCOVERED;
            flood_uncover(x - 1, y + 1);
        }

        if (y < minefield_y - 1 && minefield_visible[(y + 1) * minefield_x + x] == VIS_COVERED &&
            minefield[(y + 1) * minefield_x + x] <= 8) {
            minefield_visible[(y + 1) * minefield_x + x] = VIS_UNCOVERED;
            flood_uncover(x, y + 1);
        }

        if (x < minefield_x - 1 && minefield_visible[y * minefield_x + (x + 1)] == VIS_COVERED &&
            minefield[y * minefield_x + (x + 1)] <= 8) {
            minefield_visible[y * minefield_x + (x + 1)] = VIS_UNCOVERED;
            flood_uncover(x + 1, y);
        }
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
    int vis = minefield_visible[x + y * minefield_x];
    int min = minefield[x + y * minefield_x];
    if (vis != VIS_COVERED) return 0;
    if (min == 9) {
        if (first_move) {
            int not_mines = 0;
            for (int i = 0; i < minefield_x * minefield_y; i++) {
                if(minefield[i] != 9) {
                    minefield_unlose_buf[not_mines] = i;
                    not_mines++;
                }
            }
            if (not_mines > 0) {
                int i = minefield_unlose_buf[rand() % not_mines];
                int j = x + y * minefield_x;
                minefield[j] = 0;
                minefield[i] = 9;
                call_around(j, add_outer, -1);
                call_around(i, add_outer, 1);
                call_around(j, add_inner, 0);
                printf("res %i\n",minefield[j]);
                return make_move(x, y);
            }
        }
        for (int i = 0; i < minefield_x * minefield_y; i++) {
            if (minefield[i] == 9) minefield_visible[i] = VIS_OTHER_MINE;
            if (minefield_visible[i] == VIS_FLAG && minefield[i] != 9) minefield_visible[i] = VIS_WRONG_FLAG;
        }
        minefield_visible[x + y * minefield_x] = VIS_LOSING_MINE;
        return -1;
    } else {
        first_move = 0;
        flood_uncover(x, y);
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
