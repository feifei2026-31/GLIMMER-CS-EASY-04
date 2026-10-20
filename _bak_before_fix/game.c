
#include "game.h"

static int count_direction(const Game *g, int row, int col,
                           int dr, int dc, char player)
{
    int n = 0;
    int r = row + dr;
    int c = col + dc;

    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE
           && g->cells[r][c] == player) {
        n++;
        r += dr;
        c += dc;
    }
    return n;
}

void game_init(Game *g)
{
    int r, c;

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            g->cells[r][c] = EMPTY_CELL;
        }
    }

    g->current_player = PLAYER_X;  
    g->move_count     = 0;          
    g->status         = GAME_RUNNING;
}

int game_check_win(const Game *g, int row, int col, char player)
{
    if (count_direction(g, row, col, 0,  1, player)
      + count_direction(g, row, col, 0, -1, player) + 1 >= WIN_LENGTH) {
        return 1;
    }

    if (count_direction(g, row, col,  1, 0, player)
      + count_direction(g, row, col, -1, 0, player) + 1 >= WIN_LENGTH) {
        return 1;
    }

    if (count_direction(g, row, col,  1,  1, player)
      + count_direction(g, row, col, -1, -1, player) + 1 >= WIN_LENGTH) {
        return 1;
    }

    if (count_direction(g, row, col,  1, -1, player)
      + count_direction(g, row, col, -1,  1, player) + 1 >= WIN_LENGTH) {
        return 1;
    }

    return 0;
}

int game_is_full(const Game *g)
{
    return g->move_count >= BOARD_SIZE * BOARD_SIZE;
}

MoveResult game_place(Game *g, int row, int col)
{
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return MOVE_OUT_OF_BOUNDS;
    }

    if (g->cells[row][col] != EMPTY_CELL) {
        return MOVE_OCCUPIED;
    }

    g->cells[row][col] = g->current_player;
    g->move_count++;

    if (game_check_win(g, row, col, g->current_player)) {
        g->status = (g->current_player == PLAYER_X) ? GAME_X_WINS : GAME_O_WINS;
    } else if (game_is_full(g)) {
        g->status = GAME_DRAW;
    } else {
        g->current_player = (g->current_player == PLAYER_X) ? PLAYER_O : PLAYER_X;
    }
    return MOVE_ACCEPTED;
}
