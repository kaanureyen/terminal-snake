#include "ncurses.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define BOARD_HEIGHT (5U & 0xFF) /* uint8_t */
#define BOARD_WIDTH (5U & 0xFF) /* uint8_t */

/* ---- Position ---- */

typedef struct {
    uint8_t x; // horizontal. 0 is left
    uint8_t y; // vertical. 0 is top
} PosT;

static bool IsPosEqual(const PosT pos1, const PosT pos2)
{
    if (pos1.x == pos2.x && pos1.y == pos2.y)
        return true;
    else
        return false;
}

typedef enum {
    UP,
    RIGHT,
    DOWN,
    LEFT,
    INVALID,
} DirectionE;

static PosT GetNextPos(const PosT current_pos, const DirectionE direction)
{
    PosT next_pos = current_pos;
    switch (direction) {
    case UP:
        next_pos.y = (current_pos.y + BOARD_HEIGHT - 1) % BOARD_HEIGHT;
        break;
    case RIGHT:
        next_pos.x = (current_pos.x + 1) % BOARD_WIDTH;
        break;
    case DOWN:
        next_pos.y = (current_pos.y + 1) % BOARD_HEIGHT;
        break;
    case LEFT:
        next_pos.x = (current_pos.x + BOARD_WIDTH - 1) % BOARD_WIDTH;
        break;
    default:
        assert(false);
        break;
    }
    return next_pos;
}

/* ---- Snake ---- */

typedef struct {
    PosT body[BOARD_HEIGHT * BOARD_WIDTH]; // starts from tail.
    uint16_t length;
    DirectionE direction; // current direction of snake
} SnakeT;

static PosT GetSnakeHeadPos(const SnakeT* const Snake)
{
    return Snake->body[Snake->length - 1];
}

static PosT GetSnakeTailPos(const SnakeT* const Snake)
{
    return Snake->body[0];
}

/* ---- Board ---- */

typedef enum {
    SNAKE = 'O',
    FOOD = 'X',
    EMPTY = ' ',
} BoardE;

static void SetBoardAndUpdateDisplayBuffer(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const PosT pos, const BoardE state_to_write)
{
    board[pos.y][pos.x] = state_to_write;
    (void)mvaddch(pos.y, pos.x, state_to_write);
}

static BoardE GetBoardEnum(const BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const PosT pos)
{
    return board[pos.y][pos.x];
}

/* ---- Init ---- */

static void ResetBoard(BoardE board[BOARD_HEIGHT][BOARD_WIDTH])
{
    for (size_t x = 0; x < BOARD_WIDTH; x++)
        for (size_t y = 0; y < BOARD_HEIGHT; y++)
            SetBoardAndUpdateDisplayBuffer(board, (PosT) { .x = x, .y = y }, EMPTY);
}

static void DrawSnake(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const SnakeT* const Snake)
{
    for (size_t i = 0; i < Snake->length; i++) {
        const PosT pos_snake = Snake->body[i];
        SetBoardAndUpdateDisplayBuffer(board, pos_snake, SNAKE);
    }
}

static void DrawFood(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const PosT pos)
{
    SetBoardAndUpdateDisplayBuffer(board, pos, FOOD);
}

static void InitializeGame(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT* const Snake)
{
    ResetBoard(board);
    *Snake = (SnakeT) {
        .body = { (PosT) {
            .x = BOARD_WIDTH / 2,
            .y = BOARD_HEIGHT / 2,
        } },
        .length = 1,
        .direction = RIGHT,
    };
    DrawSnake(board, Snake);
    DrawFood(board, (PosT) {
                        .x = BOARD_WIDTH - 1,
                        .y = Snake->body[0].y,
                    });
}

/* ---- Game ---- */

static void MoveSnakeWithoutGrow(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT* const Snake, const PosT PosNewHead)
{
    SetBoardAndUpdateDisplayBuffer(board, GetSnakeTailPos(Snake), EMPTY);
    SetBoardAndUpdateDisplayBuffer(board, PosNewHead, SNAKE);

    // update snake body positions
    for (size_t i = 0; i < (size_t)(Snake->length - 1); i++)
        Snake->body[i] = Snake->body[i + 1];
    Snake->body[Snake->length - 1] = PosNewHead;
}

static void MoveSnakeWithGrow(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT* const Snake, const PosT PosFood)
{
    Snake->body[Snake->length++] = PosFood;
    SetBoardAndUpdateDisplayBuffer(board, PosFood, SNAKE);
}

static void PlaceNewFood(BoardE board[BOARD_HEIGHT][BOARD_WIDTH])
{
    PosT possble_positions[BOARD_HEIGHT * BOARD_WIDTH];
    uint16_t count_possible_positions = 0;
    for (size_t y = 0; y < BOARD_HEIGHT; y++)
        for (size_t x = 0; x < BOARD_WIDTH; x++)
            if (board[y][x] == EMPTY)
                possble_positions[count_possible_positions++] = (PosT) {
                    .x = x,
                    .y = y,
                };

    SetBoardAndUpdateDisplayBuffer(board, possble_positions[rand() % count_possible_positions], FOOD);
}

static bool CheckGameWin(const SnakeT* const Snake)
{
    // SnakeT.body has array size equal to board size. if snake size equals to board size, game is won
    return Snake->length == sizeof(Snake->body) / sizeof(Snake->body[0]);
}

static void GameOver(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT* const Snake)
{
    InitializeGame(board, Snake);
}

static void MoveSnake(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT* const Snake, const DirectionE requested_direction)
{
    // update snake direction
    switch (requested_direction) {
    case UP:
        if (Snake->direction != DOWN)
            Snake->direction = requested_direction;
        break;
    case RIGHT:
        if (Snake->direction != LEFT)
            Snake->direction = requested_direction;
        break;
    case DOWN:
        if (Snake->direction != UP)
            Snake->direction = requested_direction;
        break;
    case LEFT:
        if (Snake->direction != RIGHT)
            Snake->direction = requested_direction;
        break;
    default:
        // invalid input, noop
        break;
    }

    // process the movement
    const PosT next_pos = GetNextPos(GetSnakeHeadPos(Snake), Snake->direction);
    const BoardE next_block = GetBoardEnum(board, next_pos);
    switch (next_block) {
    case SNAKE:
        if (IsPosEqual(next_pos, GetSnakeTailPos(Snake)))
            MoveSnakeWithoutGrow(board, Snake, next_pos);
        else
            GameOver(board, Snake);
        break;
    case FOOD:
        MoveSnakeWithGrow(board, Snake, next_pos);
        if (CheckGameWin(Snake))
            GameOver(board, Snake);
        else
            PlaceNewFood(board);
        break;
    case EMPTY:
        MoveSnakeWithoutGrow(board, Snake, next_pos);
        break;
    default:
        // this means corrupt memory
        assert(false);
        break;
    }
}

/* ---- Input ---- */

void IterateGame(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT* const Snake)
{
    // consume key buffer & get the last valid input
    int last_valid_input = ' ';
    int pressed_key;
    while (ERR != (pressed_key = getch()))
        last_valid_input = pressed_key;

    DirectionE direction_request;
    switch (last_valid_input) {
    case 'w':
        direction_request = UP;
        break;
    case 'd':
        direction_request = RIGHT;
        break;
    case 's':
        direction_request = DOWN;
        break;
    case 'a':
        direction_request = LEFT;
        break;
    default:
        direction_request = INVALID;
        break;
    }
    MoveSnake(board, Snake, direction_request);
}

/* ---- Draw ---- */

void WaitFrames(uint32_t frames_to_wait)
{
    if (frames_to_wait > 60)
        frames_to_wait = 60;

    const uint32_t frame_rate = 60;
    const uint32_t ns_in_s = 1000000000;
    const uint32_t ns_to_wait = ns_in_s / frame_rate * frames_to_wait;
    const struct timespec FrameTime = {
        .tv_sec = ns_to_wait / ns_in_s,
        .tv_nsec = ns_to_wait % ns_in_s,
    };
    struct timespec RemainingTime = FrameTime;
    while (nanosleep(&RemainingTime, &RemainingTime))
        ;
}

/* ---- Main ---- */

int main(void)
{
    // init display & input
    (void)initscr(); // start curses
    (void)curs_set(0); // remove cursor
    (void)noecho(); // inhibit pressed keys showing on screen
    (void)nodelay(stdscr, TRUE); // do not wait for input

    // init game
    BoardE board[BOARD_HEIGHT][BOARD_WIDTH];
    SnakeT Snake;
    InitializeGame(board, &Snake);

    // run game
    while (true) {
        IterateGame(board, &Snake);
        (void)refresh(); // draw to screen
        WaitFrames(10); // limit frames
    }

    // uninit display
    (void)endwin(); // stop curses
    return 0;
}
