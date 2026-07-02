// ============================================================
//  MENU.ino — Game Selection Menu
// ============================================================

#define MENU_ITEMS 5
const char* menuLabels[MENU_ITEMS] = {
  "1. SNAKE",
  "2. TETRIS",
  "3. PONG",
  "4. MAZE COIN",
  "5. DONKEY KONG"
};

const uint16_t menuColors[MENU_ITEMS] = {
  0x07E0, // green  - Snake
  0x001F, // blue   - Tetris
  0xF81F, // magenta- Pong
  0xFD20, // orange - Raycaster
  0xF800  // red    - Donkey Kong
};

int menuCursor = 0;

void menuInit() {
  tft.fillScreen(ILI9341_BLACK);
  // header
  tft.fillRect(0, 0, 320, 36, ILI9341_NAVY);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(50, 10);
  tft.print("ESP32 GAME CONSOLE");
  // footer
  tft.fillRect(0, 220, 320, 20, ILI9341_DARKGREY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 226);
  tft.print("[UP/DOWN] Pilih   [FIRE] Main   [LEFT] Kembali");
  menuDraw();
}

void menuDraw() {
  for (int i = 0; i < MENU_ITEMS; i++) {
    int y = 50 + i * 32;
    bool selected = (i == menuCursor);

    // background baris
    tft.fillRect(10, y - 2, 300, 28, selected ? ILI9341_DARKGREY : ILI9341_BLACK);
    // border kiri berwarna
    tft.fillRect(10, y - 2, 4, 28, menuColors[i]);

    // arrow selector
    tft.setTextSize(2);
    tft.setTextColor(selected ? ILI9341_YELLOW : ILI9341_WHITE);
    tft.setCursor(20, y + 4);
    tft.print(selected ? "> " : "  ");
    tft.setTextColor(selected ? menuColors[i] : ILI9341_WHITE);
    tft.print(menuLabels[i]);
  }
}

void menuLoop() {
  static unsigned long lastInput = 0;
  if (millis() - lastInput < 150) return;

  bool changed = false;
  if (btnUpEdge()   && menuCursor > 0)               { menuCursor--; changed = true; }
  if (btnDownEdge() && menuCursor < MENU_ITEMS - 1)  { menuCursor++; changed = true; }
  if (changed) {
    lastInput = millis();
    menuDraw();
  }

  if (btnFireEdge()) {
    lastInput = millis();
    tft.fillScreen(ILI9341_BLACK);
    switch (menuCursor) {
      case 0: currentState = STATE_SNAKE;      snakeInit();      break;
      case 1: currentState = STATE_TETRIS;     tetrisInit();     break;
      case 2: currentState = STATE_PONG;       pongInit();       break;
      case 3: currentState = STATE_RAYCASTER;  raycasterInit();  break;
      case 4: currentState = STATE_DONKEYKONG; donkeyKongInit(); break;
    }
  }
}
