#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#define BOARD_HEIGHT (5U)
#define BOARD_WIDTH (5U)

// defined from top left as (0,0)
struct PosT
{
    unsigned char x;
    unsigned char y;
};

enum DirectionE
{
    UP,
    RIGHT,
    DOWN,
    LEFT,
};

enum BoardE
{
    SNAKE = 'O',
    FOOD = 'X',
    EMPTY = ' ',
};

struct SnakeT
{
    struct PosT body[BOARD_HEIGHT * BOARD_WIDTH]; // starts from tail.
    unsigned short length;
    enum DirectionE direction; // current direction of snake
};

static void DebugPrintBoard(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH])
{
    for (unsigned long empty_lines = 0; empty_lines < 20; empty_lines++)
        printf("\n");

    for (unsigned long y = 0; y < BOARD_HEIGHT; y++)
    {
        for (unsigned long x = 0; x < BOARD_WIDTH; x++)
        {
            printf("%c ", board[y][x]);
        }
        printf("\n");
    }
}

/* Takes current pos and direction.
    Returns the next position. */
static struct PosT GetNextPos(const struct PosT current_pos, const enum DirectionE direction)
{
    struct PosT next_pos = current_pos;
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

// this function looks dumb now. however it will be useful to update the output char by char.
static void SetBoard(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const struct PosT pos, const enum BoardE state_to_write)
{
    board[pos.y][pos.x] = state_to_write;
}

static enum BoardE GetBoardEnum(const enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const struct PosT pos)
{
    return board[pos.y][pos.x];
}

// intended for initialization.
static void ResetBoard(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH])
{
    for (unsigned long x = 0; x < BOARD_WIDTH; x++)
        for (unsigned long y = 0; y < BOARD_HEIGHT; y++)
            SetBoard(board, (struct PosT){.x = x, .y = y}, EMPTY);
}

// intended for initialization
static void DrawSnake(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const struct SnakeT *const Snake)
{
    for (unsigned long i = 0; i < Snake->length; i++)
    {
        const struct PosT pos_snake = Snake->body[i];
        SetBoard(board, pos_snake, SNAKE);
    }
}

static void DrawFood(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], const struct PosT pos)
{
    SetBoard(board, pos, FOOD);
}

static void Initialize(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], struct SnakeT *const Snake)
{
    ResetBoard(board);
    *Snake = (struct SnakeT){
        .body = {(struct PosT){
            .x = BOARD_WIDTH / 2,
            .y = 0 * BOARD_HEIGHT / 2,
        }},
        .length = 1,
        .direction = RIGHT,
    };
    DrawSnake(board, Snake);
    DrawFood(board, (struct PosT){.x = 2, .y = 4});
}

static void GameOver(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], struct SnakeT *const Snake)
{
    Initialize(board, Snake);
}

static struct PosT GetSnakeHeadPos(const struct SnakeT *const Snake)
{
    return Snake->body[Snake->length - 1];
}

static struct PosT GetSnakeTailPos(const struct SnakeT *const Snake)
{
    return Snake->body[0];
}

/* Takes snake & the position of the food it eats.
    Grows the snake.
    Updates the board.
    Returns 1 if game is won. Else returns 0. */
static bool SnakeEatFood(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], struct SnakeT *const Snake, const struct PosT PosFood)
{
    Snake->body[Snake->length++] = PosFood;
    SetBoard(board, PosFood, SNAKE);

    { // TODO: CHANGE THIS CODE BLOCK TO ACTUAL FOOD PLACEMENT
        const struct PosT next_pos = GetNextPos(GetSnakeHeadPos(Snake), Snake->direction);
        if (GetBoardEnum(board, next_pos) != SNAKE)
            SetBoard(board, next_pos, FOOD);
    }

    // check game won
    if (Snake->length == sizeof(Snake->body) / sizeof(Snake->body[0]))
        return true;
    else
        return false;
}

static void SnakeEatEmpty(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], struct SnakeT *const Snake, const struct PosT PosEmpty)
{
    SetBoard(board, GetSnakeTailPos(Snake), EMPTY);
    SetBoard(board, PosEmpty, SNAKE);
    for (unsigned long i = 0; i < Snake->length - 1; i++)
        Snake->body[i] = Snake->body[i + 1];
    Snake->body[Snake->length - 1] = PosEmpty;
}

static void MoveSnake(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], struct SnakeT *const Snake, const enum DirectionE requested_direction)
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

    const struct PosT next_pos = GetNextPos(GetSnakeHeadPos(Snake), Snake->direction);
    const enum BoardE next_block = GetBoardEnum(board, next_pos);

    switch (next_block)
    {
    case SNAKE:
        GameOver(board, Snake);
        break;
    case FOOD:
        SnakeEatFood(board, Snake, next_pos);
        break;
    case EMPTY:
        SnakeEatEmpty(board, Snake, next_pos);
        break;
    default:
        assert(false);
        break;
    }
}

void waitFrames(const unsigned long frames_to_wait)
{
    const long frame_rate = 60;

    const long ns_in_s = 1000000000;
    const long ns_to_wait = ns_in_s / frame_rate * frames_to_wait;
    const struct timespec FrameTime = {
        .tv_sec = ns_to_wait / ns_in_s,
        .tv_nsec = ns_to_wait % ns_in_s,
    };
    struct timespec RemainingTime = FrameTime;
    while (nanosleep(&RemainingTime, &RemainingTime))
        ;
}

void snakeIteration(enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH], struct SnakeT *const Snake)
{
    MoveSnake(board, Snake, DOWN);
    DebugPrintBoard(board);
    waitFrames(60);
}

int main(void)
{
    enum BoardE board[BOARD_HEIGHT][BOARD_WIDTH];
    struct SnakeT Snake;
    Initialize(board, &Snake);
    while (1)
        snakeIteration(board, &Snake);

    return 0;
}
