
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "console.h"

#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE 0x0040
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS  0x0080
#endif
/* 有些环境（VS Code / Windows Terminal）里这一位默认是开着的，
   开着时输入以 VT 转义序列形式送达，ReadConsoleInput 读不到 MOUSE_EVENT */
#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif

#define STATUS_LINE_CHARS  78

/* 把窄字符串 s 写到屏幕 (x,y) 处。
   棋盘用的是 char 缓冲区（'X' 'O' '.' '-' '|'），
   所以这里必须用 WriteConsoleOutputCharacterA（ANSI 版），
   不能用 W 版——否则 char* 被当成 wchar_t* 会打出乱码。 */
static void put(HANDLE h, int x, int y, const char *s)
{
    COORD p;
    DWORD n;
    p.X = (SHORT)x;
    p.Y = (SHORT)y;
    WriteConsoleOutputCharacterA(h, s, (DWORD)strlen(s), p, &n);
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

    /* 【关键】不要"在旧模式上打补丁"，而是显式构造我们需要的那一份：
       | 打开 | ENABLE_EXTENDED_FLAGS | 允许改快速编辑等扩展开关       |
       | 打开 | ENABLE_MOUSE_INPUT    | 让控制台上报鼠标事件           |
       | 打开 | ENABLE_PROCESSED_INPUT| 保留 Ctrl+C 能中断程序         |
       | 关闭 | QUICK_EDIT_MODE       | 否则鼠标被终端拿去"选中文字"   |
       | 关闭 | VT_INPUT (0x0200)     | 否则输入变 VT 序列，收不到鼠标 |
       | 关闭 | ECHO_INPUT / LINE_INPUT| 避免按键回显、按行缓冲干扰    | */
    mode  = ENABLE_EXTENDED_FLAGS;
    mode |= ENABLE_MOUSE_INPUT;
    mode |= ENABLE_PROCESSED_INPUT;
    mode &= ~((DWORD)ENABLE_QUICK_EDIT_MODE);
    mode &= ~((DWORD)ENABLE_VIRTUAL_TERMINAL_INPUT);
    mode &= ~((DWORD)ENABLE_ECHO_INPUT);
    mode &= ~((DWORD)ENABLE_LINE_INPUT);

    if (!SetConsoleMode(cs->hIn, mode)) return 0;

    /* 把窗口调整到能完整放下棋盘(0~18) + 状态(20~22) + 提示(24)。
       窗口太小的话，printf 会把棋盘顶出可视区域，看着就是"点不上"。 */
    {
        SMALL_RECT win;
        COORD      buf;

        SetConsoleTitleW(L"Gomoku - 五子棋");

        buf.X = 80;
        buf.Y = 300;
        SetConsoleScreenBufferSize(cs->hOut, buf);

        win.Left   = 0;
        win.Top    = 0;
        win.Right  = 79;
        win.Bottom = 29;                 /* 30 行可视高度，足够 */
        SetConsoleWindowInfo(cs->hOut, TRUE, &win);

        /* 用原生 API 清屏，不调 system("cls")（省得弹一个 cmd 子进程） */
        {
            CONSOLE_SCREEN_BUFFER_INFO info;
            COORD home;
            DWORD written, cells;

            home.X = 0;
            home.Y = 0;
            if (GetConsoleScreenBufferInfo(cs->hOut, &info)) {
                cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
                FillConsoleOutputCharacterA(cs->hOut, ' ', cells, home, &written);
                FillConsoleOutputAttribute(cs->hOut, info.wAttributes,
                                           cells, home, &written);
                SetConsoleCursorPosition(cs->hOut, home);
            }
        }

        /* 隐藏光标，别在棋盘上闪 */
        {
            CONSOLE_CURSOR_INFO ci;
            ci.dwSize   = 25;
            ci.bVisible = FALSE;
            SetConsoleCursorInfo(cs->hOut, &ci);
        }
    }

    cs->inited = 1;
    return 1;
}

/* 退出时把控制台"还原出厂设置"：恢复光标可见 + 恢复原来的输入模式
   （原来的模式里含快速编辑，恢复后鼠标又能正常选中文字了） */
void console_restore(GameConsole *cs)
{
    CONSOLE_CURSOR_INFO info;

    if (cs == NULL || !cs->inited) return;

    info.dwSize   = 20;
    info.bVisible = TRUE;
    SetConsoleCursorInfo(cs->hOut, &info);

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
            int n = (int)strlen(line);
            /* 直接按位置写入 " 子 " 三格，并补上结尾的 '\0'。
               注意：不能写成 line[strlen(line)] = cells[r][c] 之后再 strcat，
               那样会把结束符覆盖掉，导致下一次 strcat 越界读写（栈溢出）。 */
            line[n]     = ' ';
            line[n + 1] = g->cells[r][c];
            line[n + 2] = '\0';
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
            return false;          /* 读控制台失败：返回 false 让主循环退出 */
        }
        if (read == 0) continue;

        if (rec.EventType != MOUSE_EVENT) continue;

        if (rec.Event.MouseEvent.dwEventFlags != 0) continue;

        if (!(rec.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) {
            continue;
        }

        mx = rec.Event.MouseEvent.dwMousePosition.X;   
        my = rec.Event.MouseEvent.dwMousePosition.Y; 

        if (console_screen_to_cell(mx, my, row, col)) return true;
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

/* 在提示行显示一条信息（红色）。传空串即可擦掉上一次的提示。 */
void console_show_hint(GameConsole *cs, const wchar_t *msg)
{
    COORD p;
    int   i;
    int   len = (int)wcslen(msg);

    p.X = 0;
    p.Y = (SHORT)HINT_Y;
    SetConsoleCursorPosition(cs->hOut, p);

    SetConsoleTextAttribute(cs->hOut, 12);   /* 亮红 */
    printf("%ls", msg);
    for (i = len; i < 60; i++) {             /* 补空格，擦掉上一次更长的提示 */
        putchar(' ');
    }
    SetConsoleTextAttribute(cs->hOut, 7);    /* 恢复默认灰白 */
}
