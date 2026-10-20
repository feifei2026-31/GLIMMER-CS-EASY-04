#include "game.h"
#include "console.h"
int main(void)
{
    Game        game;         //定义
    GameConsole cs;         
    int         row, col;   
    MoveResult mr; 
    game_init(&game);        //初始化game
    if (!console_init(&cs)) {     //初始化gameconsole
        return 1;          
    }

    while (game.status == GAME_RUNNING) {    //当游戏正常进行时

        console_draw_board(&cs, &game); //画出棋盘边界
        console_draw_status(&cs, &game);  //判断游戏进行状态

        if (!console_wait_cell(&cs, &row, &col)) {//API
            break;
        }
        mr = game_place(&game, row, col);

        if (mr == MOVE_OCCUPIED) {
            console_show_hint(&cs, L"这个位置已经有棋子了，请换一格");
            continue;
        }

    console_draw_board(&cs, &game);   //最后全下完了再打印棋盘状态和落子数
    console_draw_status(&cs, &game);

    console_wait_key(&cs);          
    }
    return 0;
}
