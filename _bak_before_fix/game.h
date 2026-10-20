
#ifndef GAME_H
#define GAME_H

#define BOARD_SIZE   9       
#define WIN_LENGTH   5        

#define EMPTY_CELL  '.'       
#define PLAYER_X    'X'      
#define PLAYER_O    'O'     

typedef enum {
    GAME_RUNNING,      
    GAME_X_WINS,      
    GAME_O_WINS,     
    GAME_DRAW          
} GameStatus;

typedef enum {
    MOVE_ACCEPTED,       
    MOVE_OUT_OF_BOUNDS,  
    MOVE_OCCUPIED        
} MoveResult;

typedef struct {
    char       cells[BOARD_SIZE][BOARD_SIZE]; 
    char       current_player;                 
    int        move_count;                     
    GameStatus status;                         
} Game;

void game_init(Game *g);

MoveResult game_place(Game *g, int row, int col);

int game_check_win(const Game *g, int row, int col, char player);

int game_is_full(const Game *g);

#endif 
