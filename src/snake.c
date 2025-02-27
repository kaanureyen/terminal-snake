#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#define BOARD_HEIGHT (5U & 0xFF) /* uint8_t */
#define BOARD_WIDTH (5U & 0xFF)  /* uint8_t */

// defined from top left as (0,0)
typedef struct
{
    uint8_t x;
    uint8_t y;
} PosT;

typedef enum
{
    UP,
    RIGHT,
    DOWN,
    LEFT,
} DirectionE;

typedef struct
{
    PosT body[BOARD_HEIGHT * BOARD_WIDTH]; // starts from tail.
    uint16_t length;
    DirectionE direction; // current direction of snake
} SnakeT;

typedef enum
{
    SNAKE = 'O',
    FOOD = 'X',
    EMPTY = ' ',
} BoardE;

static bool IsPosEqual(const PosT pos1, const PosT pos2)
{
    if (pos1.x == pos2.x && pos1.y == pos2.y)
        return true;
    else
        return false;
}

/* Takes current pos and direction.
    Returns the next position. */
static PosT GetNextPos(const PosT current_pos, const DirectionE direction)
{
    PosT next_pos = current_pos;
    switch (direction)
    {
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

static PosT GetSnakeHeadPos(const SnakeT *const Snake)
{
    return Snake->body[Snake->length - 1];
}

static PosT GetSnakeTailPos(const SnakeT *const Snake)
{
    return Snake->body[0];
}

// this function looks dumb now. however it will be useful to update the output char by char.
static void SetBoardAndUpdate(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const PosT pos, const BoardE state_to_write)
{
    board[pos.y][pos.x] = state_to_write;
}

static BoardE GetBoardEnum(const BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const PosT pos)
{
    return board[pos.y][pos.x];
}

// intended for initialization.
static void ResetBoard(BoardE board[BOARD_HEIGHT][BOARD_WIDTH])
{
    for (size_t x = 0; x < BOARD_WIDTH; x++)
        for (size_t y = 0; y < BOARD_HEIGHT; y++)
            SetBoardAndUpdate(board, (PosT){.x = x, .y = y}, EMPTY);
}

// intended for initialization
static void DrawSnake(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const SnakeT *const Snake)
{
    for (size_t i = 0; i < Snake->length; i++)
    {
        const PosT pos_snake = Snake->body[i];
        SetBoardAndUpdate(board, pos_snake, SNAKE);
    }
}

static void DrawFood(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const PosT pos)
{
    SetBoardAndUpdate(board, pos, FOOD);
}

static void Initialize(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT *const Snake)
{
    ResetBoard(board);
    *Snake = (SnakeT){
        .body = {(PosT){
            .x = BOARD_WIDTH / 2,
            .y = BOARD_HEIGHT / 2,
        }},
        .length = 1,
        .direction = RIGHT,
    };
    DrawSnake(board, Snake);
    DrawFood(board, (PosT){
                        .x = 0,
                        .y = Snake->body[0].y,
                    });
}

static void GameOver(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT *const Snake)
{
    Initialize(board, Snake);
}

/* Takes snake & the position of the food it eats.
    Grows the snake.
    Updates the board.
    Returns 1 if game is won. Else returns 0. */
static bool MoveSnakeWithGrowAndPlaceNewFood(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT *const Snake, const PosT PosFood)
{
    Snake->body[Snake->length++] = PosFood;
    SetBoardAndUpdate(board, PosFood, SNAKE);

    { // TODO: CHANGE THIS CODE BLOCK TO ACTUAL FOOD PLACEMENT
        const PosT next_pos = GetNextPos(GetSnakeHeadPos(Snake), Snake->direction);
        if (GetBoardEnum(board, next_pos) != SNAKE)
            SetBoardAndUpdate(board, next_pos, FOOD);
    }

    // check game won
    if (Snake->length == sizeof(Snake->body) / sizeof(Snake->body[0]))
        return true;
    else
        return false;
}

static void MoveSnakeWithoutGrow(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT *const Snake, const PosT PosNewHead)
{
    SetBoardAndUpdate(board, GetSnakeTailPos(Snake), EMPTY);
    SetBoardAndUpdate(board, PosNewHead, SNAKE);

    // update snake body positions
    for (size_t i = 0; i < Snake->length - 1; i++)
        Snake->body[i] = Snake->body[i + 1];
    Snake->body[Snake->length - 1] = PosNewHead;
}

static void MoveSnake(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT *const Snake, const DirectionE requested_direction)
{
    // update direction
    switch (requested_direction)
    {
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
        assert(false);
        break;
    }

    const PosT next_pos = GetNextPos(GetSnakeHeadPos(Snake), Snake->direction);
    const BoardE next_block = GetBoardEnum(board, next_pos);

    switch (next_block)
    {
    case SNAKE:
        if (IsPosEqual(next_pos, GetSnakeTailPos(Snake)))
            MoveSnakeWithoutGrow(board, Snake, next_pos);
        else
            GameOver(board, Snake);
        break;
    case FOOD:
        if (MoveSnakeWithGrowAndPlaceNewFood(board, Snake, next_pos))
        {
            printf("\nYou Won The Snake!\n");
            exit(0);
        }
        break;
    case EMPTY:
        MoveSnakeWithoutGrow(board, Snake, next_pos);
        break;
    default:
        assert(false);
        break;
    }
}

// waits maximum 60 frames
void waitFrames(unsigned long frames_to_wait)
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

static void PrintBoard(const BoardE board[BOARD_HEIGHT][BOARD_WIDTH])
{
    printf("\n");
    for (size_t y = 0; y < BOARD_HEIGHT; y++)
    {
        for (size_t x = 0; x < BOARD_WIDTH; x++)
        {
            printf("%c ", board[y][x]);
        }
        printf("\n");
    }
}

void snakeIteration(BoardE board[BOARD_HEIGHT][BOARD_WIDTH], SnakeT *const Snake)
{
    MoveSnake(board, Snake, RIGHT);
    PrintBoard(board);
    waitFrames(20);
}

int main(void)
{
    BoardE board[BOARD_HEIGHT][BOARD_WIDTH];
    SnakeT Snake;
    Initialize(board, &Snake);
    PrintBoard(board);
    while (true)
        snakeIteration(board, &Snake);

    return 0;
}
