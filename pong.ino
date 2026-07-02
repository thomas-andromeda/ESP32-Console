// ============================================================
//  PONG.ino  — 1P vs AI (Sudah Diperbaiki dari Glitch Teks)
//  UP=paddle naik  DOWN=paddle turun  FIRE=menu
// ============================================================

#define PG_W   320
#define PG_H   240
#define PAD_W    8
#define PAD_H   40
#define BALL_S    6

// Kecepatan dikurangi dari 3.5 → 2.5
#define PG_BALL_SPEED_X  2.5f
#define PG_BALL_SPEED_Y  1.8f
#define PG_AI_SPEED      2.5f

int pgP1Y, pgAIY;
float pgBX, pgBY, pgBDX, pgBDY;
int pgScoreP, pgScoreAI;
bool pgGameOver, pgServing;
int pgWinner;
unsigned long pgServeTime;

void pgDrawField() {
  tft.fillScreen(ILI9341_BLACK);
  
  // PERBAIKAN: Membuat garis pembatas antara Scoreboard dan Area Game
  tft.drawFastHLine(0, 21, PG_W, ILI9341_DARKGREY);
  
  // PERBAIKAN: Garis tengah dimulai dari y = 24 agar tidak menabrak area skor
  for (int y = 24; y < PG_H; y += 16)
    tft.fillRect(159, y, 2, 8, ILI9341_DARKGREY);

  tft.setTextColor(ILI9341_WHITE); 
  tft.setTextSize(2);
  tft.setCursor(60, 4); tft.print(pgScoreP);
  tft.setCursor(230, 4); tft.print(pgScoreAI);
  
  // PERBAIKAN: Pindahkan info menu ke atas agar tidak terhapus bola di bawah
  tft.setTextSize(1); 
  tft.setCursor(128, 7); tft.print("[FIRE]=Menu");
}

void pgDrawPaddle(int x, int y, uint16_t color, int oldY) {
  tft.fillRect(x, oldY, PAD_W, PAD_H, ILI9341_BLACK);
  tft.fillRect(x, y,    PAD_W, PAD_H, color);
}

void pgDrawBall(int x, int y, uint16_t color) {
  tft.fillRect(x, y, BALL_S, BALL_S, color);
}

void pgStartServe(bool serveRight) {
  pgBX = PG_W / 2 - BALL_S / 2;
  pgBY = PG_H / 2 - BALL_S / 2;
  // arah Y benar-benar random, bukan hanya 2 pilihan
  float dy = PG_BALL_SPEED_Y + (random(0, 60) / 100.0f);
  if (random(0, 2)) dy = -dy;
  pgBDX = serveRight ? PG_BALL_SPEED_X : -PG_BALL_SPEED_X;
  pgBDY = dy;
  pgServing   = true;
  pgServeTime = millis();
  // tampilkan bola kuning di tengah saat jeda
  pgDrawBall((int)pgBX, (int)pgBY, ILI9341_YELLOW);
}

void pongInit() {
  pgScoreP = 0; pgScoreAI = 0; pgGameOver = false; pgServing = false;
  pgP1Y = PG_H / 2 - PAD_H / 2;
  pgAIY = PG_H / 2 - PAD_H / 2;
  pgDrawField();
  pgDrawPaddle(4, pgP1Y, ILI9341_CYAN, pgP1Y);
  pgDrawPaddle(PG_W - PAD_W - 4, pgAIY, ILI9341_RED, pgAIY);
  pgStartServe(true);
}

void pgGoGameOver(int winner) {
  pgGameOver = true; pgWinner = winner;
  tft.fillRect(80, 90, 160, 60, ILI9341_BLACK);
  tft.drawRect(80, 90, 160, 60, ILI9341_YELLOW);
  tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
  tft.setCursor(90, 100);
  tft.print(pgWinner == 1 ? "YOU WIN!" : "AI WINS!");
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.setCursor(90, 130); tft.print("[UP] Restart");
}

void pongLoop() {
  if (btnFireEdge()) { returnToMenu(); return; }

  if (pgGameOver) {
    if (btnUpEdge()) pongInit();
    return;
  }

  static unsigned long last = 0;
  if (millis() - last < 16) return;
  last = millis();

  // ── SERVE DELAY 1 DETIK ──
  if (pgServing) {
    // paddle tetap bisa digerakkan saat menunggu
    int oldP1Y = pgP1Y;
    // PERBAIKAN: Batasi batas atas gerakan paddle Player saat jeda (y > 22)
    if (btnUp()   && pgP1Y > 22)           pgP1Y -= 4;
    if (btnDown() && pgP1Y < PG_H-PAD_H)  pgP1Y += 4;
    if (pgP1Y != oldP1Y) pgDrawPaddle(4, pgP1Y, ILI9341_CYAN, oldP1Y);
    if (millis() - pgServeTime >= 1000) {
      pgServing = false;
      pgDrawBall((int)pgBX, (int)pgBY, ILI9341_BLACK);
    }
    return;
  }

  // ── ERASE BALL ──
  pgDrawBall((int)pgBX, (int)pgBY, ILI9341_BLACK);

  // ── PLAYER PADDLE ──
  int oldP1Y = pgP1Y;
  // PERBAIKAN: Batasi batas atas gerakan paddle Player utama (y > 22)
  if (btnUp()   && pgP1Y > 22)           pgP1Y -= 4;
  if (btnDown() && pgP1Y < PG_H-PAD_H)  pgP1Y += 4;
  if (pgP1Y != oldP1Y) pgDrawPaddle(4, pgP1Y, ILI9341_CYAN, oldP1Y);

  // ── AI PADDLE ──
  int oldAIY = pgAIY;
  int aiCenter   = pgAIY + PAD_H / 2;
  int ballCenter = (int)pgBY + BALL_S / 2;
  if (aiCenter < ballCenter - 4 && pgAIY < PG_H - PAD_H) pgAIY += (int)PG_AI_SPEED;
  // PERBAIKAN: Batasi batas atas gerakan paddle AI (y > 22)
  if (aiCenter > ballCenter + 4 && pgAIY > 22)             pgAIY -= (int)PG_AI_SPEED;
  if (pgAIY != oldAIY) pgDrawPaddle(PG_W-PAD_W-4, pgAIY, ILI9341_RED, oldAIY);

  // ── MOVE BALL ──
  pgBX += pgBDX;
  pgBY += pgBDY;

  // PERBAIKAN: Batasi pantulan atas bola agar memantul di bawah garis skor (y <= 22)
  if (pgBY <= 22)           { pgBY = 22; pgBDY =  fabsf(pgBDY); }
  if (pgBY >= PG_H-BALL_S) { pgBY = PG_H-BALL_S; pgBDY = -fabsf(pgBDY); }

  // player paddle collision
  if (pgBX <= 4 + PAD_W && pgBX >= 2 &&
      pgBY + BALL_S >= pgP1Y && pgBY <= pgP1Y + PAD_H) {
    pgBX = 4 + PAD_W + 1;
    pgBDX = min(fabsf(pgBDX) * 1.05f, 6.0f);
    float rel = ((pgBY + BALL_S/2.0f) - (pgP1Y + PAD_H/2.0f)) / (PAD_H/2.0f);
    pgBDY = rel * 3.5f;
  }

  // AI paddle collision
  int aiX = PG_W - PAD_W - 4;
  if (pgBX + BALL_S >= aiX && pgBX <= aiX + PAD_W + 2 &&
      pgBY + BALL_S >= pgAIY && pgBY <= pgAIY + PAD_H) {
    pgBX = aiX - BALL_S - 1;
    pgBDX = -min(fabsf(pgBDX) * 1.05f, 6.0f);
    float rel = ((pgBY + BALL_S/2.0f) - (pgAIY + PAD_H/2.0f)) / (PAD_H/2.0f);
    pgBDY = rel * 3.5f;
  }

  // ── SCORING ──
  if (pgBX < 0) {
    pgScoreAI++;
    pgDrawField();
    pgDrawPaddle(4, pgP1Y, ILI9341_CYAN, pgP1Y);
    pgDrawPaddle(PG_W-PAD_W-4, pgAIY, ILI9341_RED, pgAIY);
    if (pgScoreAI >= 7) { pgGoGameOver(2); return; }
    pgStartServe(true);
    return;
  }
  if (pgBX > PG_W) {
    pgScoreP++;
    pgDrawField();
    pgDrawPaddle(4, pgP1Y, ILI9341_CYAN, pgP1Y);
    pgDrawPaddle(PG_W-PAD_W-4, pgAIY, ILI9341_RED, pgAIY);
    if (pgScoreP >= 7) { pgGoGameOver(1); return; }
    pgStartServe(false);
    return;
  }

  pgDrawBall((int)pgBX, (int)pgBY, ILI9341_WHITE);
}