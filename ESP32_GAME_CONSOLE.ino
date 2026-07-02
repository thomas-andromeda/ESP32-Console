// ============================================================
//  ESP32 GAME CONSOLE — Main Entry Point
//  Board  : ESP32 DOIT DevKit
//  Display: ILI9341 320x240
//  Libs   : Adafruit_GFX, Adafruit_ILI9341
// ============================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// ─── PIN DEFINITIONS ─────────────────────────────────────────
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4

#define BTN_UP    12
#define BTN_DOWN  14
#define BTN_LEFT  27
#define BTN_RIGHT 26
#define BTN_FIRE  13

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ─── GAME STATE ──────────────────────────────────────────────
enum GameState { STATE_MENU, STATE_SNAKE, STATE_TETRIS,
                 STATE_PONG, STATE_RAYCASTER, STATE_DONKEYKONG };
GameState currentState = STATE_MENU;

// ─── BUTTON HELPERS ──────────────────────────────────────────
bool btnUp()    { return digitalRead(BTN_UP)    == LOW; }
bool btnDown()  { return digitalRead(BTN_DOWN)  == LOW; }
bool btnLeft()  { return digitalRead(BTN_LEFT)  == LOW; }
bool btnRight() { return digitalRead(BTN_RIGHT) == LOW; }
bool btnFire()  { return digitalRead(BTN_FIRE)  == LOW; }

// debounce helpers
bool btnUpEdge()    { static bool p=false; bool c=btnUp();    bool e=c&&!p; p=c; return e; }
bool btnDownEdge()  { static bool p=false; bool c=btnDown();  bool e=c&&!p; p=c; return e; }
bool btnLeftEdge()  { static bool p=false; bool c=btnLeft();  bool e=c&&!p; p=c; return e; }
bool btnRightEdge() { static bool p=false; bool c=btnRight(); bool e=c&&!p; p=c; return e; }
bool btnFireEdge()  { static bool p=false; bool c=btnFire();  bool e=c&&!p; p=c; return e; }

// ─── UTILITY FUNCTIONS ───────────────────────────────────────
// Fungsi helper otomatis untuk membuat teks berada di tengah layar
void printCentered(const char* text, int y, int size, uint16_t color) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  
  int16_t textWidth = strlen(text) * 6 * size; 
  int16_t x = (320 - textWidth) / 2;
  
  if (x < 0) x = 0; 
  tft.setCursor(x, y);
  tft.print(text);
}

// ─── SETUP ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_FIRE,  INPUT_PULLUP);

  tft.begin();
  tft.setRotation(1); // landscape 320x240
  tft.fillScreen(ILI9341_BLACK);

  showSplash();
  menuInit();
}

// ─── LOOP ────────────────────────────────────────────────────
void loop() {
  switch (currentState) {
    case STATE_MENU:       menuLoop();       break;
    case STATE_SNAKE:      snakeLoop();      break;
    case STATE_TETRIS:     tetrisLoop();     break;
    case STATE_PONG:       pongLoop();       break;
    case STATE_RAYCASTER:  raycasterLoop();  break;
    case STATE_DONKEYKONG: donkeyKongLoop(); break;
  }
}

// ─── SPLASH SCREEN ───────────────────────────────────────────
void showSplash() {
  tft.fillScreen(ILI9341_BLACK);
  
  printCentered("ESP32 GAME", 60, 3, ILI9341_YELLOW);
  printCentered("CONSOLE", 100, 3, ILI9341_YELLOW);
  printCentered("5 GAMES IN 1 DEVICE", 150, 1, ILI9341_CYAN);
  printCentered("Press FIRE to start", 200, 1, ILI9341_WHITE);

  while (!btnFire()) delay(10);
  delay(200);
}

// ─── RETURN TO MENU (called from any game) ───────────────────
void returnToMenu() {
  currentState = STATE_MENU;
  menuInit();
}