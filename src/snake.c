#if defined(_WIN32) || defined(WIN32)
#include "ncurses/ncurses.h"
#else
#include "ncurses.h"
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define BOARD_HEIGHT (5U & 0xFF) /* uint8_t */
#define BOARD_WIDTH (5U & 0xFF) /* uint8_t */

/* ---- Position & Direction ---- */

typedef struct {
    uint8_t x; // 0 is left
    uint8_t y; // 0 is top
} Pos_t;

bool IsPosEqual(const Pos_t Pos1, const Pos_t Pos2)
{
    return Pos1.x == Pos2.x && Pos1.y == Pos2.y;
}

typedef enum {
    UP,
    RIGHT,
    DOWN,
    LEFT,
    INVALID,
} direction_t;

Pos_t GetNextPos(const Pos_t CurrentPos, const direction_t direction)
{
    Pos_t NextPos = CurrentPos;
    switch (direction) {
    case UP:
        NextPos.y = (CurrentPos.y + BOARD_HEIGHT - 1) % BOARD_HEIGHT;
        break;
    case RIGHT:
        NextPos.x = (CurrentPos.x + 1) % BOARD_WIDTH;
        break;
    case DOWN:
        NextPos.y = (CurrentPos.y + 1) % BOARD_HEIGHT;
        break;
    case LEFT:
        NextPos.x = (CurrentPos.x + BOARD_WIDTH - 1) % BOARD_WIDTH;
        break;
    default:
        assert(false);
        break;
    }
    return NextPos;
}

/* ---- Board ---- */

typedef enum {
    SNAKE = 'O',
    FOOD = 'X',
    EMPTY = ' ',
} board_t;

void SetBoardAndUpdateDisplayBuffer(board_t board[BOARD_HEIGHT][BOARD_WIDTH], const Pos_t Pos, const board_t state_to_write)
{
    board[Pos.y][Pos.x] = state_to_write;
    (void)mvaddch(Pos.y, Pos.x, state_to_write);
}

board_t GetBoardState(const board_t board[BOARD_HEIGHT][BOARD_WIDTH], const Pos_t Pos)
{
    return board[Pos.y][Pos.x];
}

/* ---- Snake ---- */

typedef struct {
    Pos_t Body[BOARD_HEIGHT * BOARD_WIDTH]; // starts from tail.
    uint16_t length;
    direction_t direction; // current direction of snake
} Snake_t;

Pos_t GetSnakeHeadPos(const Snake_t* const Snake)
{
    return Snake->Body[Snake->length - 1];
}

Pos_t GetSnakeTailPos(const Snake_t* const Snake)
{
    return Snake->Body[0];
}

void UpdateSnakeDirection(Snake_t* const Snake, const direction_t requested_direction)
{
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
}

/* ---- Init ---- */

static void ResetBoard(board_t board[BOARD_HEIGHT][BOARD_WIDTH])
{
    Pos_t Pos;
    for (Pos.y = 0; Pos.y < BOARD_HEIGHT; Pos.y++)
        for (Pos.x = 0; Pos.x < BOARD_WIDTH; Pos.x++)
            SetBoardAndUpdateDisplayBuffer(board, Pos, EMPTY);
}

static void DrawSnake(board_t board[BOARD_HEIGHT][BOARD_WIDTH], const Snake_t* const Snake)
{
    for (size_t i = 0; i < Snake->length; i++)
        SetBoardAndUpdateDisplayBuffer(board, Snake->Body[i], SNAKE);
}

void InitializeGame(board_t board[BOARD_HEIGHT][BOARD_WIDTH], Snake_t* const Snake)
{
    ResetBoard(board);

    *Snake = (Snake_t) {
        .Body = { (Pos_t) {
            .x = BOARD_WIDTH / 2,
            .y = BOARD_HEIGHT / 2,
        } },
        .length = 1,
        .direction = RIGHT,
    };
    DrawSnake(board, Snake);

    SetBoardAndUpdateDisplayBuffer(board,
        (Pos_t) {
            .x = BOARD_WIDTH - 1,
            .y = Snake->Body[0].y,
        },
        FOOD);
}

/* ---- Game ---- */

typedef enum {
    CONTINUING,
    WON,
    LOST,
} game_t;

static void MoveSnakeWithoutGrow(board_t board[BOARD_HEIGHT][BOARD_WIDTH], Snake_t* const Snake, const Pos_t PosNewHead)
{
    SetBoardAndUpdateDisplayBuffer(board, GetSnakeTailPos(Snake), EMPTY);
    SetBoardAndUpdateDisplayBuffer(board, PosNewHead, SNAKE);

    // update snake Body positions
    for (size_t i = 0; i < (size_t)(Snake->length - 1); i++)
        Snake->Body[i] = Snake->Body[i + 1];
    Snake->Body[Snake->length - 1] = PosNewHead;
}

static void MoveSnakeWithGrow(board_t board[BOARD_HEIGHT][BOARD_WIDTH], Snake_t* const Snake, const Pos_t PosFood)
{
    Snake->Body[Snake->length++] = PosFood;
    SetBoardAndUpdateDisplayBuffer(board, PosFood, SNAKE);
}

static void PlaceNewFood(board_t board[BOARD_HEIGHT][BOARD_WIDTH])
{
    Pos_t PossiblePositions[BOARD_HEIGHT * BOARD_WIDTH];
    uint16_t count_possible_positions = 0;

    {
        Pos_t CandidatePos = { 0 };
        for (CandidatePos.y = 0; CandidatePos.y < BOARD_HEIGHT; CandidatePos.y++)
            for (CandidatePos.x = 0; CandidatePos.x < BOARD_WIDTH; CandidatePos.x++)
                if (GetBoardState(board, CandidatePos) == EMPTY)
                    PossiblePositions[count_possible_positions++] = CandidatePos;
    }

    assert(count_possible_positions >= 1);
    SetBoardAndUpdateDisplayBuffer(board, PossiblePositions[rand() % count_possible_positions], FOOD);
}

static bool IsGameWin(const Snake_t* const Snake)
{
    // Snake_t.Body has array size equal to board size. if snake size equals to board size, game is won
    return Snake->length == sizeof(Snake->Body) / sizeof(Snake->Body[0]);
}

game_t IterateGame(board_t board[BOARD_HEIGHT][BOARD_WIDTH], Snake_t* const Snake, const direction_t requested_direction)
{
    UpdateSnakeDirection(Snake, requested_direction);

    const Pos_t NextPos = GetNextPos(GetSnakeHeadPos(Snake), Snake->direction);
    const board_t next_block = GetBoardState(board, NextPos);
    switch (next_block) {
    case SNAKE:
        if (IsPosEqual(NextPos, GetSnakeTailPos(Snake)))
            MoveSnakeWithoutGrow(board, Snake, NextPos);
        else
            return LOST;
        break;
    case FOOD:
        MoveSnakeWithGrow(board, Snake, NextPos);
        if (IsGameWin(Snake))
            return WON;
        else
            PlaceNewFood(board);
        break;
    case EMPTY:
        MoveSnakeWithoutGrow(board, Snake, NextPos);
        break;
    default:
        // this means corrupt memory
        assert(false);
        break;
    }

    return CONTINUING;
}

/* ---- Input ---- */

int ReadLastInput(void)
{
    int last_valid_input = ERR;

    int pressed_key;
    while (ERR != (pressed_key = getch()))
        last_valid_input = pressed_key;

    return last_valid_input;
}

bool ReadYesNoInput(void)
{
    (void)nodelay(stdscr, FALSE);

    while (true) {
        const int pressed_key = getch();
        switch (pressed_key) {
        case 'Y':
        case 'y':
            (void)nodelay(stdscr, TRUE);
            return true;
            break;
        case 'N':
        case 'n':
            (void)nodelay(stdscr, TRUE);
            return false;
            break;
        }
    }
}

bool ContinueQuestion(const char* sentence)
{
    mvaddstr(BOARD_HEIGHT, 0, sentence);

    bool answer = ReadYesNoInput();

    clear();
    return answer;
}

direction_t GetDirectionRequest(int input)
{
    switch (input) {
    case 'W':
    case 'w':
    case KEY_UP:
        return UP;
        break;
    case 'D':
    case 'd':
    case KEY_RIGHT:
        return RIGHT;
        break;
    case 'S':
    case 's':
    case KEY_DOWN:
        return DOWN;
        break;
    case 'A':
    case 'a':
    case KEY_LEFT:
        return LEFT;
        break;
    default:
        return INVALID;
        break;
    }
}

/* ---- Draw ---- */

void WaitFrames(uint32_t frames_to_wait)
{
    // saturate
    if (frames_to_wait > 60)
        frames_to_wait = 60;

    // calculate time to wait
    const uint32_t frame_rate = 60;
    const uint32_t ns_in_s = 1000000000;
    const uint32_t ns_to_wait = ns_in_s / frame_rate * frames_to_wait;
    const struct timespec FrameTime = {
        .tv_sec = ns_to_wait / ns_in_s,
        .tv_nsec = ns_to_wait % ns_in_s,
    };

    // wait it
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
    board_t board[BOARD_HEIGHT][BOARD_WIDTH];
    Snake_t Snake;
    InitializeGame(board, &Snake);

    // run game
    bool continue_game = true;
    while (continue_game) {
        switch (IterateGame(board, &Snake, GetDirectionRequest(ReadLastInput()))) {
        case WON:
            if (ContinueQuestion("You won! Continue? (y/n)")) {
                InitializeGame(board, &Snake);
                (void)refresh();
            } else
                continue_game = false;
            break;
        case LOST:
            if (ContinueQuestion("You lost! Continue? (y/n)")) {
                InitializeGame(board, &Snake);
                (void)refresh();
            } else
                continue_game = false;
            break;
        case CONTINUING:
            (void)refresh(); // draw to screen
            WaitFrames(10); // limit frames
            break;
        default:
            assert(false);
            break;
        }
    }

    // uninit display
    (void)endwin(); // stop curses
    return 0;
}
