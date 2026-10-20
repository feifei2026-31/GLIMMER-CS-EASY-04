#include "game.h"
#include "console.h"

int main(void)
{
    Game        game;         /* 游戏数据 */
    GameConsole cs;           /* 终端状态 */
    int         row, col;     /* 鼠标点中的棋盘坐标 */
    MoveResult  mr;

    game_init(&game);                       /* 初始化 game */
    if (!console_init(&cs)) {               /* 初始化控制台，确认真的在终端里 */
        return 1;
    }

    while (game.status == GAME_RUNNING) {   /* 还没分出胜负就一直下 */

        console_draw_board(&cs, &game);     /* 画棋盘 */
        console_draw_status(&cs, &game);    /* 画底部状态 */

        if (!console_wait_cell(&cs, &row, &col)) {  /* 等一次有效的格子点击 */
            console_restore(&cs);                   /* 控制台读取失败：收尾退出 */
            return 1;
        }

        mr = game_place(&game, row, col);           /* 落子 */

        if (mr == MOVE_OCCUPIED) {                  /* 这一格已经有棋子 */
            console_show_hint(&cs, L"这个位置已经有棋子了，请换一格");
            continue;                               /* 不换手，重新等点击 */
        }
        if (mr == MOVE_OUT_OF_BOUNDS) {             /* 点在棋盘外 */
            continue;
        }

        console_show_hint(&cs, L"");                /* 落子成功，擦掉旧提示 */
    }

    console_draw_board(&cs, &game);         /* 收尾：把最终盘面留在屏幕上 */
    console_draw_status(&cs, &game);
    console_wait_key(&cs);                  /* 等一次按键（按任意键退出） */
    console_restore(&cs);                   /* 恢复控制台状态 */

    return 0;
}
