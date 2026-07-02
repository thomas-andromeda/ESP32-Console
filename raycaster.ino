#define RC_W      320
#define RC_H      200   // area 3D
#define RC_HALF   100
#define RC_MAPSZ   16  // 16x16 map

// Kecepatan jalan dan rotasi yang sudah ditingkatkan (lebih cepat)
#define MOVE_SPD  0.14f   
#define ROT_SPD   0.065f  
#define MAX_COINS  5

// ── MAP (generated at runtime) ──────────────────────────────
uint8_t rcMap[RC_MAPSZ][RC_MAPSZ];

// ── PLAYER ──────────────────────────────────────────────────
float rcPX, rcPY;     // posisi
float rcDX, rcDY;     // direction vector
float rcPlX, rcPlY;   // camera plane

// ── COINS ───────────────────────────────────────────────────
struct Coin { float x, y; bool collected; bool isTrap; };
Coin rcCoins[MAX_COINS];
int  rcCoinsLeft; 

bool rcWon;
bool rcLost; 
bool rcJumpscareDone;
unsigned long rcWinTime;

// ── OPTIMIZATION & HORROR CONFIG ────────────────────────────
float rcZBuffer[RC_W]; 
int rcOldPdx = -1;
int rcOldPdy = -1;

// ── MAP GENERATION (Random Walk + Border) ───────────────────
void rcGenMap() {
  for (int y=0;y<RC_MAPSZ;y++)
    for (int x=0;x<RC_MAPSZ;x++)
      rcMap[y][x] = 1;

  int wx = RC_MAPSZ/2, wy = RC_MAPSZ/2;
  int steps = RC_MAPSZ * RC_MAPSZ * 2;
  rcMap[wy][wx] = 0;

  for (int s=0;s<steps;s++) {
    int dir = random(0,4);
    int nx=wx, ny=wy;
    if (dir==0) ny--;
    else if (dir==1) ny++;
    else if (dir==2) nx--; else nx++;

    if (nx>=1 && nx<RC_MAPSZ-1 && ny>=1 && ny<RC_MAPSZ-1) {
      wx=nx;
      wy=ny;
      rcMap[wy][wx]=0;
    }
  }

  for (int i=0;i<RC_MAPSZ;i++) {
    rcMap[0][i]=1;
    rcMap[RC_MAPSZ-1][i]=1;
    rcMap[i][0]=1; rcMap[i][RC_MAPSZ-1]=1;
  }

  rcMap[1][1]=0; rcMap[2][1]=0; rcMap[1][2]=0;
}

// ── COIN PLACEMENT ──────────────────────────────────────────
void rcPlaceCoins() {
  int placed = 0;
  int tries  = 0;

  while (placed < MAX_COINS && tries < 500) {
    tries++;
    int cx = random(1, RC_MAPSZ-1);
    int cy = random(1, RC_MAPSZ-1);
    if (rcMap[cy][cx] != 0) continue;
    if (cx<=2 && cy<=2) continue;

    bool overlap=false;
    for (int i=0; i<placed; i++)
      if (fabsf(rcCoins[i].x-(cx+0.5f))<1.2f && fabsf(rcCoins[i].y-(cy+0.5f))<1.2f) { 
        overlap=true;
        break; 
      }
    if (overlap) continue;

    rcCoins[placed].x = cx + 0.5f;
    rcCoins[placed].y = cy + 0.5f;
    rcCoins[placed].collected = false;
    
    if (placed == 0 || placed == 3) {
      rcCoins[placed].isTrap = true;
    } else {
      rcCoins[placed].isTrap = false;
    }
    
    placed++;
  }
  for (int i=placed; i<MAX_COINS; i++) {
    rcCoins[i].x=2.5f;
    rcCoins[i].y=2.5f;
    rcCoins[i].collected=true; 
    rcCoins[i].isTrap = false;
  }
}

// ── WALL SHADING (HORROR UPDATE: Flickering & Steeper Falloff) ──
uint16_t rcWallColor(int side, float dist) {
  // 1. Efek Kedipan Senter Rusak (Random Flashlight Flicker)
  int flicker = 0;
  if (random(0, 100) > 96) { 
    flicker = random(40, 90); // Jatuhkan kecerahan secara mendadak
  }

  // 2. Jarak pandang dibuat jauh lebih pendek (Exaggerated Fog)
  // Di atas jarak ~4 unit, lorong akan menjadi gelap gulita (Claustrophobia)
  int br = (int)(230.0f / (1.0f + dist * 0.55f)) - flicker;
  br = constrain(br, 0, 210);
  if (br < 15) br = 0; // Kegelapan absolut di ujung lorong

  if (side==1) br = br * 5 / 10; // Bayangan dinding samping dibuat lebih kontras

  // 3. Ubah palet warna menjadi Merah Karat Kusam / Darah Kering (Dread Tone)
  uint8_t r = (br * 22) / 255, g = (br * 3) / 255, b = (br * 2) / 255;
  return ((r&0x1F)<<11)|((g&0x3F)<<5)|(b&0x1F);
}

// ── RENDER FRAME (HORROR UPDATE: Pitch Black Void) ──────────
void rcRenderFrame() {
  uint16_t colBuf[RC_H]; 

  tft.startWrite();
  for (int x=0; x<RC_W; x++) {
    float camX = 2.0f*x/(float)RC_W - 1.0f;
    float rdx  = rcDX + rcPlX*camX;
    float rdy  = rcDY + rcPlY*camX;

    int mx=(int)rcPX, my=(int)rcPY;
    float ddx = fabsf(rdx)<1e-6f ? 1e30f : fabsf(1.0f/rdx);
    float ddy = fabsf(rdy)<1e-6f ? 1e30f : fabsf(1.0f/rdy);
    float sdx,sdy; int stx,sty,side=0;

    if (rdx<0){stx=-1;sdx=(rcPX-mx)*ddx;}else{stx=1;sdx=(mx+1.0f-rcPX)*ddx;}
    if (rdy<0){sty=-1;sdy=(rcPY-my)*ddy;}else{sty=1;sdy=(my+1.0f-rcPY)*ddy;}

    int safe=0;
    while(safe++<60){
      if(sdx<sdy){sdx+=ddx;mx+=stx;side=0;}else{sdy+=ddy;my+=sty;side=1;}
      if(mx<0||mx>=RC_MAPSZ||my<0||my>=RC_MAPSZ){break;}
      if(rcMap[my][mx]>0) break;
    }
    float dist=(side==0)?(sdx-ddx):(sdy-ddy);
    if(dist<0.05f) dist=0.05f;

    rcZBuffer[x] = dist;

    int lh=(int)(RC_H/dist);
    int ds=RC_HALF-lh/2, de=RC_HALF+lh/2;
    int drawS=max(ds,0), drawE=min(de,RC_H-1);

    uint16_t wc=rcWallColor(side,dist);

    // HORROR UPDATE: Langit-langit dan lantai dibuat hitam pekat (Void effect)[cite: 1]
    for (int y = 0; y < drawS; y++)       colBuf[y] = 0x0000; // Gelap gulita[cite: 1]
    for (int y = drawS; y <= drawE; y++)  colBuf[y] = wc;     // Dinding karat/kedip[cite: 1]
    for (int y = drawE + 1; y < RC_H; y++) colBuf[y] = 0x0821; // Lantai semen mati basah[cite: 1]

    tft.setAddrWindow(x, 0, 1, RC_H);
    tft.writePixels(colBuf, RC_H);
  }
  tft.endWrite();
}

// ── DRAW COIN ───────────────────────────────────────────────
void rcDrawCoins() {
  for (int i=0;i<MAX_COINS;i++) {
    if (rcCoins[i].collected) continue;
    float cx = rcCoins[i].x - rcPX;
    float cy = rcCoins[i].y - rcPY;

    float invDet = 1.0f/(rcPlX*rcDY - rcDX*rcPlY);
    float tx = invDet*(rcDY*cx - rcDX*cy);
    float tz = invDet*(-rcPlY*cx + rcPlX*cy);

    if (tz <= 0.1f) continue;
    int sprX = (int)((RC_W/2)*(1.0f + tx/tz));
    int sprH = min((int)(RC_H/tz), RC_H);
    int sprW = sprH; // Proporsi koin bulat pas
    
    int drawSX = max(sprX - sprW/2, 0);
    int drawEX = min(sprX + sprW/2, RC_W-1);
    int drawSY = max(RC_HALF - sprH/2, 0);
    int drawEY = min(RC_HALF + sprH/2, RC_H-1);

    if (drawEX<=drawSX || drawEY<=drawSY) continue;
    if (sprW<2||sprH<2) continue;

    uint16_t cc;
    if (rcCoins[i].isTrap) {
      cc = ILI9341_RED; // Trap koin berwarna merah (tampak mencurigakan)
    } else {
      cc = tz<3.0f ? 0xFFE0 : 0xC400; // Koin asli kuning kusam
    }
    
    for (int stripe = drawSX; stripe <= drawEX; stripe++) {
      if (tz < rcZBuffer[stripe]) {
        tft.fillRect(stripe, drawSY, 1, drawEY - drawSY, cc);
      }
    }
  }
}

// ── HUD (HORROR UPDATE: Malfunctioning Radar) ───────────────
void rcDrawHUD(bool forceRedrawAll) {
  int mx=240, my=RC_H+4;

  // Cek apakah ada koin jebakan/setan di radius dekat pemain (Indera Keenam)
  bool nearTrap = false;
  for (int i = 0; i < MAX_COINS; i++) {
    if (!rcCoins[i].collected && rcCoins[i].isTrap) {
      float dx = rcCoins[i].x - rcPX;
      float dy = rcCoins[i].y - rcPY;
      if ((dx*dx + dy*dy) < 6.25f) { // Radius sekitar 2.5 blok map
        nearTrap = true;
        break;
      }
    }
  }

  if (forceRedrawAll) {
    tft.fillRect(0,RC_H,RC_W,40,ILI9341_BLACK);
    tft.drawRect(0,RC_H,RC_W,40,ILI9341_DARKGREY);
    tft.setTextColor(ILI9341_RED); tft.setTextSize(1); // Teks HUD diubah jadi merah ketakutan
    tft.setCursor(8,RC_H+6);
    tft.print("REMAINING SAMPLES: "); // Mengubah istilah koin menjadi objek fiksi ilmiah/horor
    tft.setTextColor(ILI9341_WHITE);  tft.print(rcCoinsLeft); 

    tft.setTextColor(ILI9341_DARKGREY);
    tft.setCursor(8,RC_H+20);
    tft.print("[FIRE]=Menu  [UP/DN]=Walk  [LT/RT]=Turn");
  }

  // LOGIKA HORROR RADAR: Jika dekat koin jebakan, radar akan korsleting/berkedip merah agresif!
  if (nearTrap && (millis() % 300 < 150)) {
    // Efek radar rusak karena interfensi mistis/bahaya
    tft.fillRect(mx, my, RC_MAPSZ*2, RC_MAPSZ*2, 0x5800); // Kotak radar berkedip merah tua
  } else {
    // Redraw peta dasar (Hanya dinding, posisi objektif/koin disembunyikan total agar pemain buta posisi koin!)
    if (forceRedrawAll || nearTrap) { 
      for (int r=0;r<RC_MAPSZ;r++) {
        for (int c=0;c<RC_MAPSZ;c++) {
          uint16_t col=rcMap[r][c]?0x2104:ILI9341_BLACK; // Dinding dibuat sangat samudra gelap
          tft.fillRect(mx+c*2, my+r*2, 2,2, col);
        }
      }
    }
  }

  // Hapus dot jejak pemain yang lama
  if (rcOldPdx >= mx && rcOldPdy >= my) {
    tft.fillRect(rcOldPdx, rcOldPdy, 2, 2, ILI9341_BLACK); 
  }
  
  // Gambar dot posisi pemain saat ini jika radar sedang tidak korsleting merah
  int pdx=mx+(int)(rcPX*2), pdy=my+(int)(rcPY*2);
  pdx=constrain(pdx,mx,mx+RC_MAPSZ*2-2);
  pdy=constrain(pdy,my,my+RC_MAPSZ*2-2);
  
  if (!(nearTrap && (millis() % 300 < 150))) {
    tft.fillRect(pdx,pdy,2,2,ILI9341_GREEN); // Pemain tetap hijau murni
  }
  
  rcOldPdx = pdx;
  rcOldPdy = pdy;
}

// ── JUMPSCARE — Wajah Monster Pixel Art ─────────────────────
const uint8_t MONSTER_FACE[18][24] PROGMEM = {
  {0,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,0},
  {0,0,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0}, // Retakan di dahi kiri
  {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1}, // Mata diperlebar ke atas
  {1,2,2,3,3,2,2,1,1,1,1,1,1,1,1,1,1,2,2,3,3,2,2,1}, // Pupil '3' melotot besar
  {1,2,2,3,3,2,2,1,1,1,1,1,1,1,1,1,1,2,2,3,3,2,2,1},
  {1,1,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,1,1},
  {1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,1,1,1}, // Efek darah mengalir dari mata
  {1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,1,1,1}, // (lanjutan aliran darah)
  {1,1,1,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1}, // Seringai lebar dimulai
  {1,1,1,1,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,1,1,1,1,1}, // Taring atas (selang-seling 1 dan 3)
  {1,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1}, // Rongga mulut hitam
  {1,1,1,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,1,1,1,1}, // Taring bawah
  {1,1,1,1,1,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1}, // Mulut robek ke rahang
  {1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1}, // Pipi cekung/membusuk
  {1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1},
  {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
};

void rcShowJumpscare(bool won) {
  if (!won) {
    for (int f=0;f<3;f++) {
      tft.fillScreen(ILI9341_RED);
      delay(80);
      tft.fillScreen(ILI9341_BLACK);
      delay(60);
    }
    
    int bsz = 13;
    int offX = (320 - 24*bsz)/2;
    int offY = (240 - 18*bsz)/2;
    tft.fillScreen(ILI9341_BLACK);
    for (int r=0;r<18;r++) {
      for (int c=0;c<24;c++) {
        uint8_t v = pgm_read_byte(&MONSTER_FACE[r][c]);
        uint16_t col;
        if      (v==0) col=ILI9341_BLACK;
        else if (v==1) col=ILI9341_RED;
        else if (v==2) col=ILI9341_WHITE;
        else           col=0x07E0;
        tft.fillRect(offX+c*bsz, offY+r*bsz, bsz, bsz, col);
      }
    }
    
    for (int f=0;f<4;f++) {
      tft.fillScreen(ILI9341_RED);
      delay(60);
      for (int r=0;r<18;r++)
        for (int c=0;c<24;c++) {
          uint8_t v=pgm_read_byte(&MONSTER_FACE[r][c]);
          uint16_t col=v==0?ILI9341_BLACK:v==1?ILI9341_RED:v==2?ILI9341_WHITE:0x07E0;
          tft.fillRect(offX+c*bsz,offY+r*bsz,bsz,bsz,col);
        }
      delay(120);
    }
    delay(1000);
  }
  
  tft.fillScreen(ILI9341_BLACK);
  if (won) {
    tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(3);
    tft.setCursor(60,80);
    tft.print("YOU WIN!");
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
    tft.setCursor(60,130); tft.print("All Valid Samples Secured!");
    tft.setCursor(60,150); tft.print("[FIRE] = Menu");
  } else {
    tft.setTextColor(ILI9341_RED); tft.setTextSize(3);
    tft.setCursor(60,80);
    tft.print("YOU DIED!");
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
    tft.setCursor(50,130);
    tft.print("The entity found you.");
    tft.setCursor(50,150); tft.print("[FIRE] = Try Again");
  }
}

// ── COIN PICKUP CHECK ───────────────────────────────────────
void rcCheckCoins() {
  for (int i=0;i<MAX_COINS;i++) {
    if (rcCoins[i].collected) continue;
    float dx = rcCoins[i].x - rcPX;
    float dy = rcCoins[i].y - rcPY;
    if (dx*dx + dy*dy < 0.4f) {
      rcCoins[i].collected = true;
      if (rcCoins[i].isTrap) {
        rcLost = true; 
        delay(200);
        rcShowJumpscare(false);
        return;
      } else {
        rcCoinsLeft--;
        rcDrawHUD(true); 
      }
    }
  }
}

// ── INIT ────────────────────────────────────────────────────
void raycasterInit() {
  randomSeed(millis());
  rcGenMap();
  rcPlaceCoins();
  
  rcWon = false;
  rcLost = false; 
  rcJumpscareDone = false;

  rcCoinsLeft=0;
  for(int i=0;i<MAX_COINS;i++) {
    if(!rcCoins[i].collected && !rcCoins[i].isTrap) rcCoinsLeft++;
  }

  rcPX=1.5f;
  rcPY=1.5f;
  rcDX=1.0f; rcDY=0.0f;
  rcPlX=0.0f; rcPlY=0.66f;
  
  rcOldPdx = -1;
  rcOldPdy = -1;

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.setCursor(90,115); tft.print("Calibrating systems...");
  rcRenderFrame();
  rcDrawCoins();
  rcDrawHUD(true); 
}

// ── LOOP ────────────────────────────────────────────────────
void raycasterLoop() {
  if (rcLost) {
    if (btnFireEdge()) {
      raycasterInit();
    }
    return;
  }

  if (btnFireEdge()) { 
    returnToMenu(); 
    return;
  }

  if (rcWon) return;

  bool moved = false;
  
  if (btnUp()) {
    float nx = rcPX + rcDX * MOVE_SPD;
    float ny = rcPY + rcDY * MOVE_SPD;
    if (nx >= 0 && nx < RC_MAPSZ && rcMap[(int)rcPY][(int)nx] == 0) rcPX = nx;
    if (ny >= 0 && ny < RC_MAPSZ && rcMap[(int)ny][(int)rcPX] == 0) rcPY = ny;
    moved = true;
  }
  if (btnDown()) {
    float nx = rcPX - rcDX * MOVE_SPD;
    float ny = rcPY - rcDY * MOVE_SPD;
    if (nx >= 0 && nx < RC_MAPSZ && rcMap[(int)rcPY][(int)nx] == 0) rcPX = nx;
    if (ny >= 0 && ny < RC_MAPSZ && rcMap[(int)ny][(int)rcPX] == 0) rcPY = ny;
    moved = true;
  }
  if (btnLeft()) {
    float ca=cosf(ROT_SPD),sa=sinf(-ROT_SPD);
    float odx=rcDX,opx=rcPlX;
    rcDX=rcDX*ca-rcDY*sa; rcDY=odx*sa+rcDY*ca;
    rcPlX=rcPlX*ca-rcPlY*sa; rcPlY=opx*sa+rcPlY*ca;
    moved=true;
  }
  if (btnRight()) {
    float ca=cosf(-ROT_SPD),sa=sinf(ROT_SPD);
    float odx=rcDX,opx=rcPlX;
    rcDX=rcDX*ca-rcDY*sa; rcDY=odx*sa+rcDY*ca;
    rcPlX=rcPlX*ca-rcPlY*sa; rcPlY=opx*sa+rcPlY*ca;
    moved=true;
  }

  // HORROR UPDATE: Selalu update HUD untuk memproses animasi kedipan radar bahaya[cite: 1]
  rcDrawHUD(false);

  if (moved) {
    rcCheckCoins();
    if (rcLost) return; 
    
    rcRenderFrame();
    rcDrawCoins();
    
    if (rcCoinsLeft <= 0 && !rcWon) {
      rcWon = true;
      delay(300);
      rcShowJumpscare(true);
    }
  }
}