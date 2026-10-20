#ifndef CONSOLE_H
#define CONSOLE_H

#include <windows.h>
#include "game.h"
#include <stdbool.h>

#define BOARD_PITCH_X   4
#define BOARD_PITCH_Y   2

#define BOARD_WIDTH   (1 + BOARD_SIZE * BOARD_PITCH_X)

#define BOARD_HEIGHT  (1 + BOARD_SIZE * BOARD_PITCH_Y)

#define STATUS_Y      (BOARD_HEIGHT + 1)

typedef struct {
    HANDLE hIn;         
    HANDLE hOut;        
    DWORD  old_mode;  
    int    inited;     
} GameConsole;

int  console_init(GameConsole *cs);

void console_restore(GameConsole *cs);

void console_draw_board(const GameConsole *cs, const Game *g);

void console_draw_status(const GameConsole *cs, const Game *g);

int  console_screen_to_cell(int mx, int my, int *row, int *col);

void console_show_hint(GameConsole *cs, const wchar_t *msg);

bool console_wait_cell(const GameConsole *cs, int *row, int *col);

bool console_wait_key(const GameConsole *cs);

void console_restore(GameConsole *cs);

#endif

