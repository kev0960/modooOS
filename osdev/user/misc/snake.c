#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

#define MAP_WIDTH 200
#define MAP_HEIGHT 200
#define MAX_LEN 100
#define RECT_SIZE 10

struct Pos {
  int row;
  int col;
};

enum Direction { UP, RIGHT, DOWN, LEFT };

struct Snake {
  struct Pos pos[MAX_LEN];
  int current_len;
  enum Direction dir;
  struct Pos food;
};

void Draw(struct Snake* snake, int* vm) {
  for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++) {
    vm[i] = 0;
  }

  for (int i = 0; i < snake->current_len; i++) {
    int draw_row = snake->pos[i].row * RECT_SIZE;
    int draw_col = snake->pos[i].col * RECT_SIZE;

    for (int h = 0; h < RECT_SIZE; h++) {
      for (int w = 0; w < RECT_SIZE; w++) {
        vm[(draw_row + h) * MAP_WIDTH + (draw_col + w)] = 0xff00ff;
      }
    }
  }

  int draw_row = snake->food.row * RECT_SIZE;
  int draw_col = snake->food.col * RECT_SIZE;

  for (int h = 0; h < RECT_SIZE; h++) {
    for (int w = 0; w < RECT_SIZE; w++) {
      vm[(draw_row + h) * MAP_WIDTH + (draw_col + w)] = 0x00ffff;
    }
  }

  struct FrameBufferInfo binfo;
  binfo.screen_row = 10;
  binfo.screen_col = 10;
  binfo.buffer_width = MAP_WIDTH;
  binfo.buffer_height = MAP_HEIGHT;

  screen(COPY_FRAME_BUFFER, vm, &binfo);
}

bool MoveSnake(struct Snake* snake, enum Direction dir) {
  for (int i = snake->current_len - 1; i > 0; i--) {
    snake->pos[i] = snake->pos[i - 1];
  }

  switch (dir) {
    case UP:
      snake->pos[0].row--;
      break;
    case RIGHT:
      snake->pos[0].col++;
      break;
    case DOWN:
      snake->pos[0].row++;
      break;
    case LEFT:
      snake->pos[0].col--;
      break;
  }

  if (snake->pos[0].row < 0 || snake->pos[0].row >= MAP_HEIGHT / RECT_SIZE) {
    return false;
  }

  if (snake->pos[0].col < 0 || snake->pos[0].col >= MAP_WIDTH / RECT_SIZE) {
    return false;
  }

  return true;
}

bool MakeLonger(struct Snake* snake) {
  for (int i = snake->current_len - 1; i >= 0; i--) {
    snake->pos[i + 1] = snake->pos[i];
  }

  switch (snake->dir) {
    case UP:
      snake->pos[0].row--;
      break;
    case RIGHT:
      snake->pos[0].col++;
      break;
    case DOWN:
      snake->pos[0].row++;
      break;
    case LEFT:
      snake->pos[0].col--;
      break;
  }

  if (snake->pos[0].row < 0 || snake->pos[0].row >= MAP_HEIGHT / RECT_SIZE) {
    return false;
  }

  if (snake->pos[0].col < 0 || snake->pos[0].col >= MAP_WIDTH / RECT_SIZE) {
    return false;
  }

  snake->current_len++;
  return true;
}

int main() {
  console(SET_NO_BUFFER | SET_NON_BLOCKING_IO);

  int* vm = (int*)malloc(MAP_WIDTH * MAP_HEIGHT * sizeof(int));

  struct Snake snake;
  snake.pos[0].row = 4;
  snake.pos[0].col = 4;
  snake.current_len = 1;

  snake.food.row = 9;
  snake.food.col = 9;

  size_t last_action = mstick();
  size_t last_draw = mstick();
  while (1) {
    char buf[10];
    int cnt = read(0, buf, 10);
    if (cnt != 0) {
      bool res = true;
      if (buf[0] == 'w') {
        res = MoveSnake(&snake, UP);
        snake.dir = UP;
      } else if (buf[0] == 'a') {
        res = MoveSnake(&snake, LEFT);
        snake.dir = LEFT;
      } else if (buf[0] == 'd') {
        res = MoveSnake(&snake, RIGHT);
        snake.dir = RIGHT;
      } else if (buf[0] == 's') {
        res = MoveSnake(&snake, DOWN);
        snake.dir = DOWN;
      }

      if (!res) {
        break;
      }
    }

    if (snake.pos[0].col == snake.food.col &&
        snake.pos[0].row == snake.food.row) {
      bool res = MakeLonger(&snake);
      snake.food.col = (snake.food.col + 3) % (MAP_WIDTH / RECT_SIZE);
      snake.food.row = (snake.food.row + 7) % (MAP_WIDTH / RECT_SIZE);
      if (!res) break;
    }

    size_t cur = mstick();
    if (cur - last_action >= 600) {
      bool res = MoveSnake(&snake, snake.dir);
      if (!res) {
        break;
      }
      last_action = mstick();
    }

    if (cur - last_draw >= 100) {
      // Refersh vm.
      Draw(&snake, vm);
      last_draw = mstick();
    }
  }

  return 0;
}
