// ============================================================
//  TETRIS.ino
//  UP=rotate DOWN=soft drop LEFT=kiri RIGHT=kanan FIRE=menu
// ============================================================

#define TT_COLS   10
#define TT_ROWS   20
#define TT_CELL   11
#define TT_OX     60    // Modifikasi: Diubah ke 60 agar posisi board dan sidebar center di layar 320x240
#define TT_OY     10

uint8_t ttBoard[TT_ROWS][TT_COLS];
int ttScore, ttLines, ttLevel;
bool ttGameOver;
bool ttGameOverDrawn = false; 
unsigned long ttLastFall;
int ttFallSpeed;

// piece shape: 7 tetrominoes, 4 rotations each, 4 cells each
// Format: {dx,dy} relative to pivot
const int8_t PIECES[7][4][4][2] PROGMEM = {
  // I
  {{{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}},
   {{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}},
  // O
  {{{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}},
   {{0,0},{1,0},{0,1},{1,1}}, {{0,0},{1,0},{0,1},{1,1}}},
  // T
  {{{-1,0},{0,0},{1,0},{0,-1}}, {{0,-1},{0,0},{1,0},{0,1}},
   {{-1,0},{0,0},{1,0},{0,1}},  {{0,-1},{0,0},{-1,0},{0,1}}},
  // S
  {{{-1,0},{0,0},{0,-1},{1,-1}}, {{0,-1},{0,0},{1,0},{1,1}},
   {{-1,0},{0,0},{0,-1},{1,-1}}, {{0,-1},{0,0},{1,0},{1,1}}},
  // Z
  {{{-1,-1},{0,-1},{0,0},{1,0}}, {{1,-1},{0,0},{1,0},{0,1}},
   {{-1,-1},{0,-1},{0,0},{1,0}}, {{1,-1},{0,0},{1,0},{0,1}}},
  // J
  {{{-1,-1},{-1,0},{0,0},{1,0}}, {{0,-1},{1,-1},{0,0},{0,1}},
   {{-1,0},{0,0},{1,0},{1,1}},   {{0,-1},{0,0},{-1,1},{0,1}}},
  // L
  {{{-1,0},{0,0},{1,0},{1,-1}},  {{0,-1},{0,0},{0,1},{1,1}},
   {{-1,1},{-1,0},{0,0},{1,0}},  {{-1,-1},{0,-1},{0,0},{0,1}}}
};

const uint16_t PIECE_COLORS[7] = {
  0x07FF, // I cyan
  0xFFE0, // O yellow
  0xF81F, // T magenta
  0x07E0, // S green
  0xF800, // Z red
  0x001F, // J blue
  0xFD20  // L orange
};

int8_t ttPX, ttPY, ttPType, ttPRot;
int8_t ttNextType;

void ttGetCells(int8_t type, int8_t rot, int8_t px, int8_t py, int8_t out[4][2]) {
  for (int i = 0; i < 4; i++) {
    out[i][0] = px + pgm_read_byte(&PIECES[type][rot][i][0]);
    out[i][1] = py + pgm_read_byte(&PIECES[type][rot][i][1]);
  }
}

bool ttValid(int8_t type, int8_t rot, int8_t px, int8_t py) {
  int8_t cells[4][2];
  ttGetCells(type, rot, px, py, cells);
  for (int i = 0; i < 4; i++) {
    int x = cells[i][0], y = cells[i][1];
    if (x < 0 || x >= TT_COLS || y >= TT_ROWS) return false;
    if (y >= 0 && ttBoard[y][x]) return false;
  }
  return true;
}

void ttDrawCell(int x, int y, uint16_t color) {
  int px = TT_OX + x * TT_CELL;
  int py = TT_OY + y * TT_CELL;
  tft.fillRect(px, py, TT_CELL-1, TT_CELL-1, color);
}

void ttDrawPiece(bool erase) {
  int8_t cells[4][2];
  ttGetCells(ttPType, ttPRot, ttPX, ttPY, cells);
  for (int i = 0; i < 4; i++) {
    if (cells[i][1] >= 0)
      ttDrawCell(cells[i][0], cells[i][1], erase ? ILI9341_BLACK : PIECE_COLORS[ttPType]);
  }
}

void ttDrawBoard() {
  for (int y = 0; y < TT_ROWS; y++)
    for (int x = 0; x < TT_COLS; x++)
      ttDrawCell(x, y, ttBoard[y][x] ? PIECE_COLORS[ttBoard[y][x]-1] : ILI9341_BLACK);
}

void ttSpawnPiece() {
  ttPType = ttNextType;
  ttNextType = random(0, 7);
  ttPRot = 0;
  ttPX = TT_COLS / 2;
  ttPY = 0;
  if (!ttValid(ttPType, ttPRot, ttPX, ttPY)) ttGameOver = true;
}

void ttLockPiece() {
  int8_t cells[4][2];
  ttGetCells(ttPType, ttPRot, ttPX, ttPY, cells);
  for (int i = 0; i < 4; i++)
    if (cells[i][1] >= 0) ttBoard[cells[i][1]][cells[i][0]] = ttPType + 1;
}

void ttClearLines() {
  int cleared = 0;
  for (int y = TT_ROWS - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < TT_COLS; x++) if (!ttBoard[y][x]) { full=false; break; }
    if (full) {
      cleared++;
      for (int r = y; r > 0; r--)
        memcpy(ttBoard[r], ttBoard[r-1], TT_COLS);
      memset(ttBoard[0], 0, TT_COLS);
      y++; 
    }
  }
  if (cleared) {
    static const int pts[5] = {0,100,300,500,800};
    ttScore += pts[min(cleared,4)] * (ttLevel + 1);
    ttLines += cleared;
    ttLevel  = ttLines / 10;
    ttFallSpeed = max(80, 500 - ttLevel * 40);
    ttDrawBoard();
  }
}

void ttUpdateSidebar() {
  int sx = TT_OX + TT_COLS * TT_CELL + 10;
  tft.fillRect(sx, 10, 80, 180, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(sx, 10);  tft.print("SCORE");
  tft.setCursor(sx, 20);  tft.setTextColor(ILI9341_YELLOW); tft.print(ttScore);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(sx, 40);  tft.print("LINES");
  tft.setCursor(sx, 50);  tft.setTextColor(ILI9341_CYAN);   tft.print(ttLines);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(sx, 70);  tft.print("LEVEL");
  tft.setCursor(sx, 80);  tft.setTextColor(ILI9341_GREEN);  tft.print(ttLevel);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(sx, 100); tft.print("NEXT");
  
  // Preview kotak background dark grey
  tft.fillRect(sx, 110, 44, 44, ILI9341_DARKGREY);
  
  // Penyesuaian pivot agar balok panjang (I) dan kotak (O) pas di tengah kotak preview
  int8_t previewPX = (ttNextType == 0) ? 1 : 2;
  int8_t previewPY = (ttNextType == 1) ? 1 : 2;
  
  int8_t cells[4][2];
  ttGetCells(ttNextType, 0, previewPX, previewPY, cells);
  for (int i = 0; i < 4; i++)
    tft.fillRect(sx + cells[i][0]*11, 110 + cells[i][1]*11,
                 10, 10, PIECE_COLORS[ttNextType]);
                 
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.setCursor(sx, 160); tft.print("[FIRE]");
  tft.setCursor(sx, 170); tft.print("Menu");
}

void tetrisInit() {
  memset(ttBoard, 0, sizeof(ttBoard));
  ttScore = 0;
  ttLines = 0; ttLevel = 0;
  ttFallSpeed = 500; ttGameOver = false;
  ttGameOverDrawn = false; 
  ttNextType = random(0, 7);
  tft.fillScreen(ILI9341_BLACK);
  tft.drawRect(TT_OX-1, TT_OY-1, TT_COLS*TT_CELL+2, TT_ROWS*TT_CELL+2, ILI9341_WHITE);
  ttSpawnPiece();
  ttUpdateSidebar();
  ttDrawPiece(false);
  ttLastFall = millis();
}

void tetrisLoop() {
  if (btnFireEdge()) { returnToMenu(); return; }
  
  if (ttGameOver) {
    if (!ttGameOverDrawn) { 
      // Modifikasi: Posisi box X digeser ke 75 agar pop-up presisi di tengah layar (lebar 320)
      int goX = 75; 
      tft.fillRect(goX, 90, 170, 60, ILI9341_BLACK);
      tft.drawRect(goX, 90, 170, 60, ILI9341_RED);
      
      // Mengatur posisi teks Game Over agar center align di dalam pop-up box
      tft.setTextColor(ILI9341_RED); tft.setTextSize(2);
      tft.setCursor(goX + 31, 100); tft.print("GAME OVER"); 
      
      tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
      
      // Menghitung dinamis posisi tengah teks skor berdasarkan jumlah digit angka
      int scoreTextWidth = 42 + (ttScore == 0 ? 6 : (String(ttScore).length() * 6));
      tft.setCursor(goX + (170 - scoreTextWidth) / 2, 125); 
      tft.print("Score: "); tft.print(ttScore);
      
      tft.setCursor(goX + 49, 138); tft.print("[UP] Restart");
      ttGameOverDrawn = true; 
    }
    if (btnUpEdge()) tetrisInit();
    return;
  }

  static unsigned long lastInput = 0;
  
  // Input pergerakan horizontal & rotasi block
  if (millis() - lastInput > 120) {
    bool moved = false;
    if (btnLeftEdge()  && ttValid(ttPType, ttPRot, ttPX-1, ttPY)) { ttDrawPiece(true); ttPX--; moved=true; }
    if (btnRightEdge() && ttValid(ttPType, ttPRot, ttPX+1, ttPY)) { ttDrawPiece(true); ttPX++; moved=true; }
    if (btnUpEdge()) {
      int8_t nr = (ttPRot + 1) % 4;
      if (ttValid(ttPType, nr, ttPX, ttPY)) { ttDrawPiece(true); ttPRot = nr; moved=true; }
    }
    if (moved) { ttDrawPiece(false); lastInput = millis(); }
  }

  // Soft drop sistem turun cepat
  bool softDrop = btnDown();
  unsigned long fallInterval = softDrop ? 50 : (unsigned long)ttFallSpeed;
  
  if (millis() - ttLastFall > fallInterval) {
    ttLastFall = millis();
    if (ttValid(ttPType, ttPRot, ttPX, ttPY+1)) {
      ttDrawPiece(true); ttPY++; ttDrawPiece(false);
    } else {
      ttLockPiece();
      ttClearLines();
      ttSpawnPiece();
      ttUpdateSidebar(); 
      if (!ttGameOver) ttDrawPiece(false);
    }
  }
}