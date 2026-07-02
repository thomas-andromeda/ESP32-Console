// ============================================================
//  SNAKE.ino
//  UP=naik DOWN=turun LEFT=kiri RIGHT=kanan FIRE=kembali menu
// ============================================================

#define SN_COLS   31
#define SN_ROWS   23
#define SN_CELL    8
#define SN_OX     16  // offset X (centering)
#define SN_OY     20  // offset Y

#define SN_MAX    200
#define ILI9341_LIME 0x87E0

struct SnakeCell { int8_t x, y; };

SnakeCell snBody[SN_MAX];
int  snLen;
int8_t snDX, snDY;
int8_t foodX, foodY;
int  snScore;
bool snGameOver;
unsigned long snLastMove;
int  snSpeed; // ms per move

void snakePlaceFood() {
  bool ok = false;
  while (!ok) {
    foodX = random(0, SN_COLS);
    foodY = random(0, SN_ROWS);
    ok = true;
    for (int i = 0; i < snLen; i++)
      if (snBody[i].x == foodX && snBody[i].y == foodY) { ok = false; break; }
  }
  // draw food
  tft.fillRect(SN_OX + foodX * SN_CELL, SN_OY + foodY * SN_CELL,
               SN_CELL - 1, SN_CELL - 1, ILI9341_RED);
}

void snakeDrawBorder() {
  tft.drawRect(SN_OX - 1, SN_OY - 1,
               SN_COLS * SN_CELL + 2, SN_ROWS * SN_CELL + 2, ILI9341_WHITE);
  // header
  tft.fillRect(0, 0, 320, 18, ILI9341_NAVY);
  tft.setTextColor(ILI9341_GREEN); tft.setTextSize(1);
  tft.setCursor(4, 5); tft.print("SNAKE");
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(60, 5); tft.print("Score:");
  tft.setCursor(110, 5); tft.print(snScore);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(220, 5); tft.print("[FIRE]=Menu");
}

void snakeInit() {
  tft.fillScreen(ILI9341_BLACK);
  snLen   = 4;
  snDX    = 1; snDY = 0;
  snScore = 0;
  snSpeed = 150;
  snGameOver = false;
  snLastMove = millis();

  int sx = SN_COLS / 2, sy = SN_ROWS / 2;
  for (int i = 0; i < snLen; i++) { snBody[i].x = sx - i; snBody[i].y = sy; }

  snakeDrawBorder();
  // draw initial snake
  for (int i = 0; i < snLen; i++) {
    uint16_t c = (i == 0) ? ILI9341_LIME : ILI9341_GREEN;
    tft.fillRect(SN_OX + snBody[i].x * SN_CELL, SN_OY + snBody[i].y * SN_CELL,
                 SN_CELL - 1, SN_CELL - 1, c);
  }
  snakePlaceFood();
}

void snakeLoop() {
  if (btnFireEdge()) { returnToMenu(); return; }
  if (snGameOver) {
    if (btnFireEdge() || btnUpEdge()) snakeInit();
    return;
  }

  // direction input (no 180 reversal)
  if (btnUpEdge()    && snDY != 1)  { snDX = 0;  snDY = -1; }
  if (btnDownEdge()  && snDY != -1) { snDX = 0;  snDY =  1; }
  if (btnLeftEdge()  && snDX != 1)  { snDX = -1; snDY =  0; }
  if (btnRightEdge() && snDX != -1) { snDX =  1; snDY =  0; }

  if (millis() - snLastMove < (unsigned long)snSpeed) return;
  snLastMove = millis();

  // erase tail
  tft.fillRect(SN_OX + snBody[snLen-1].x * SN_CELL,
               SN_OY + snBody[snLen-1].y * SN_CELL,
               SN_CELL - 1, SN_CELL - 1, ILI9341_BLACK);

  // shift body
  for (int i = snLen - 1; i > 0; i--) snBody[i] = snBody[i-1];

  // move head
  snBody[0].x += snDX;
  snBody[0].y += snDY;

  // wall collision
  if (snBody[0].x < 0 || snBody[0].x >= SN_COLS ||
      snBody[0].y < 0 || snBody[0].y >= SN_ROWS) { snakeGameOver(); return; }

  // self collision
  for (int i = 1; i < snLen; i++)
    if (snBody[i].x == snBody[0].x && snBody[i].y == snBody[0].y) { snakeGameOver(); return; }

  // food eaten
  if (snBody[0].x == foodX && snBody[0].y == foodY) {
    snLen = min(snLen + 1, SN_MAX);
    snScore += 10;
    snSpeed  = max(60, snSpeed - 3);
    // redraw score
    tft.fillRect(110, 4, 80, 10, ILI9341_NAVY);
    tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
    tft.setCursor(110, 5); tft.print(snScore);
    snakePlaceFood();
  }

  // draw head
  tft.fillRect(SN_OX + snBody[0].x * SN_CELL, SN_OY + snBody[0].y * SN_CELL,
               SN_CELL - 1, SN_CELL - 1, ILI9341_LIME);
  // recolor neck
  if (snLen > 1)
    tft.fillRect(SN_OX + snBody[1].x * SN_CELL, SN_OY + snBody[1].y * SN_CELL,
                 SN_CELL - 1, SN_CELL - 1, ILI9341_GREEN);
}

void snakeGameOver() {
  snGameOver = true;
  tft.fillRect(60, 90, 200, 60, ILI9341_BLACK);
  tft.drawRect(60, 90, 200, 60, ILI9341_RED);
  tft.setTextColor(ILI9341_RED);   tft.setTextSize(2);
  tft.setCursor(80, 100); tft.print("GAME OVER");
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.setCursor(70, 125); tft.print("Score: "); tft.print(snScore);
  tft.setCursor(65, 138); tft.print("[UP/FIRE] Restart");
}
