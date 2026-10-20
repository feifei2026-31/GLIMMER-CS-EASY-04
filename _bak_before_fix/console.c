
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "console.h"

#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE 0x0040
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS  0x0080
#endif

#define STATUS_LINE_CHARS  78

static void put(HANDLE h, int x, int y, const wchar_t *s)
{
    COORD p;
    DWORD n;
    p.X = (SHORT)x;
    p.Y = (SHORT)y;
    WriteConsoleOutputCharacterW(h, s, (DWORD)wcslen(s), p, &n);
}

static void make_border(char *line)
{
    int c;

    strcpy(line, "+");//strcpy = 字符串复制
    for (c = 0; c < BOARD_SIZE; c++) {
        strcat(line, "---+");
    }
}

int console_init(GameConsole *cs)
{
    DWORD mode = 0;

    cs->inited = 0;
    cs->hIn  = GetStdHandle(STD_INPUT_HANDLE);
    cs->hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (cs->hIn == INVALID_HANDLE_VALUE || cs->hOut == INVALID_HANDLE_VALUE) {
        return 0;
    }


    if (!GetConsoleMode(cs->hIn, &cs->old_mode)) return 0;

    mode  = cs->old_mode;
    mode &= ~((DWORD)ENABLE_QUICK_EDIT_MODE);
    mode |= ENABLE_EXTENDED_FLAGS;     
    mode |= ENABLE_MOUSE_INPUT;        
    SetConsoleMode(cs->hIn, mode);

    system("cls");                     
    cs->inited = 1;
    return 1;
}

void console_restore(GameConsole *cs)
{
    if (!cs->inited) return;
    SetConsoleMode(cs->hIn, cs->old_mode);  
    cs->inited = 0;
}

void console_draw_board(const GameConsole *cs, const Game *g)
{
    char border[BOARD_WIDTH + 1];
    char line[BOARD_WIDTH + 1];
    int  r, c, y;

    make_border(border);

    put(cs->hOut, 0, 0, border);

    for (r = 0; r < BOARD_SIZE; r++) {

        y = 1 + BOARD_PITCH_Y * r;    

        strcpy(line, "|");
        for (c = 0; c < BOARD_SIZE; c++) {
            strcat(line, " ");
            line[strlen(line)] = g->cells[r][c];  
            strcat(line, " |");
        }
        put(cs->hOut, 0, y, line);

        put(cs->hOut, 0, y + 1, border);
    }
}

void console_draw_status(const GameConsole *cs, const Game *g)
{
    COORD p;

    p.X = 0;
    p.Y = (SHORT)STATUS_Y;
    SetConsoleCursorPosition(cs->hOut, p);

    if (g->status == GAME_RUNNING) {
        printf("Player: %c                \n", g->current_player);
    } else if (g->status == GAME_X_WINS) {
        printf("Game over: X wins!        \n");
    } else if (g->status == GAME_O_WINS) {
        printf("Game over: O wins!        \n");
    } else {
        printf("Game over: draw.          \n");
    }

    printf("Moves:   %d / %d           \n", g->move_count, BOARD_SIZE * BOARD_SIZE);

    if (g->status == GAME_RUNNING) {
        printf("Left-click a cell to place.  \n");
    } else {
        printf("Press any key to exit.       \n");
    }
}


int console_screen_to_cell(int mx, int my, int *row, int *col)
{
    if (mx < 0 || mx >= BOARD_SIZE * BOARD_PITCH_X) return 0;
    if (my < 0 || my >= BOARD_SIZE * BOARD_PITCH_Y) return 0;

    *col = mx / BOARD_PITCH_X;
    *row = my / BOARD_PITCH_Y;
    return 1;
}

bool console_wait_cell(const GameConsole *cs, int *row, int *col)
{
    INPUT_RECORD rec;
    DWORD        read;
    int          mx, my;

    for (;;) {
        if (!ReadConsoleInputW(cs->hIn, &rec, 1, &read)) {
            *row = -1;            
            *col = -1;
            return;
        }
        if (read == 0) continue;

        if (rec.EventType != MOUSE_EVENT) continue;

        if (rec.Event.MouseEvent.dwEventFlags != 0) continue;

        if (!(rec.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
            continue;
        }

        mx = rec.Event.MouseEvent.dwMousePosition.X;   
        my = rec.Event.MouseEvent.dwMousePosition.Y; 

        if (console_screen_to_cell(mx, my, row, col)) return;
    }
}

void console_wait_key(const GameConsole *cs)
{
    INPUT_RECORD rec;
    DWORD        read;

    for (;;) {
        if (!ReadConsoleInputW(cs->hIn, &rec, 1, &read)) return;
        if (read == 0) continue;
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) return;
    }
}

void console_show_hint(GameConsole *cs, const wchar_t *msg) {
    goto_xy(0, 20);                   
    SetConsoleTextAttribute(cs->hOut, 12);   
    printf("%ls", msg);                
    SetConsoleTextAttribute(cs->hOut, 7);
}

void console_restore(GameConsole *cs)
{
    if (cs == NULL || !cs->inited) return;

    /* 恢复光标可见 */
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 20;
    info.bVisible = TRUE;
    SetConsoleCursorInfo(cs->hOut, &info);

    /* 恢复控制台默认模式（重新启用快速编辑） */
    DWORD mode = 0;
    GetConsoleMode(cs->hIn, &mode);
    mode |= ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(cs->hIn, mode);
}