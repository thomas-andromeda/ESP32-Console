// ============================================================
//  DONKEY_KONG.ino  v5 — Stable 5-Floor Layout (Fixed Holes & Hitbox)
//  Platform collision: snap ke atas platform
//  Ladder mechanics: smooth multi-ladder snapping
//  Barrel: slower roll, stable roll (FAIR RANDOM LADDER)
// ============================================================

// ── LAYOUT ──────────────────────────────────────────────────
#define DK_ARENA_TOP  20
#define DK_W          320
#define DK_H          200

// ── PLATFORM (Sisi kanan Lantai 4 diperpanjang penuh agar tidak bolong) ──
#define NUM_PLAT 5
struct DKPlatform { int16_t x, y, w; bool right; };
const DKPlatform DKP[NUM_PLAT] = {
  {  0, 190, 320,  true },  // 0 Ground (Satu-satunya jalan keluar barrel di kanan bawah)
  {  0, 153, 300,  true },  // 1 Bergetar ke kanan, jatuh di kanan (300-320)
  { 20, 116, 300, false },  // 2 Bergerak ke kiri, jatuh di kiri (0-20)
  {  0,  79, 300,  true },  // 3 Bergerak ke kanan, jatuh di kanan (300-320)
  { 20,  42, 300, false },  // 4 Top Floor (Bergerak ke KIRI, Sisi kanan penuh hingga x=320 untuk Princess)
};

// ── LADDER (8 Tangga Menghubungkan 5 Lantai secara Presisi) ──
#define NUM_LAD 8
struct DKLadder { int16_t x, yTop, yBot; };
const DKLadder DKL[NUM_LAD] = {
  {  60, 153, 190 }, // Lantai 0 ke 1 (Kiri)
  { 240, 153, 190 }, // Lantai 0 ke 1 (Kanan)
  { 120, 116, 153 }, // Lantai 1 ke 2 (Tengah)
  { 260, 116, 153 }, // Lantai 1 ke 2 (Kanan)
  {  50,  79, 116 }, // Lantai 2 ke 3 (Kiri)
  { 200,  79, 116 }, // Lantai 2 ke 3 (Kanan)
  { 100,  42,  79 }, // Lantai 3 ke 4 (Tengah)
  { 220,  42,  79 }, // Lantai 3 ke 4 (Kanan menuju area Princess)
};

// ── MARIO ───────────────────────────────────────────────────
#define MR_W  12
#define MR_H  20
#define MR_SPEED      1.6f
#define MR_JUMP      -6.8f
#define MR_GRAV       0.48f
#define MR_LAD_SPEED  1.2f  // Kecepatan memanjat dikurangi agar butuh skill timing

struct DKMario {
  float x, y;
  float vy;
  bool  grounded;
  bool  onLadder;
  bool  jumping;
  int   anim;
  unsigned long animT;
};
DKMario mkMario;

// ── DK ──────────────────────────────────────────────────────
struct DKKong {
  int16_t x, y;
  bool throwing;
  int  throwTick;
  unsigned long animT;
  int  anim;
};
DKKong mkDK;

// ── BARREL ──────────────────────────────────────────────────
#define MAX_BR 15
#define BR_W   12
#define BR_H   10
struct DKBarrel {
  float x, y;
  float vx, vy;
  bool  active;
  bool  onLadder;
  int   platIdx;
  int   anim;
  unsigned long animT;
  int   lastLadderCheck; 
};
DKBarrel mkBR[MAX_BR];

// ── STATE ────────────────────────────────────────────────────
int  dkScore, dkLives, dkLevel;
bool dkGameOver;
unsigned long dkBarrelTimer, dkTick;
int  dkBarrelMs;

// ── COLORS ──────────────────────────────────────────────────
#define DKC_BG    0x0000
#define DKC_PLAT  0x8410
#define DKC_LAD   0xFD20
#define DKC_MB    0xF800   
#define DKC_MS    0xFFE0   
#define DKC_DK    0x8200
#define DKC_BR    0xC980
#define DKC_PAUL  0xF81F

// ── DRAW UTILS ──────────────────────────────────────────────
void dkR(int x, int y, int w, int h, uint16_t c) {
  if (w<=0||h<=0) return;
  tft.fillRect(x, DK_ARENA_TOP+y, w, h, c);
}

// ── DRAW ARENA ──────────────────────────────────────────────
void dkDrawArena() {
  tft.fillRect(0, DK_ARENA_TOP, DK_W, DK_H, DKC_BG);
  for (int i=0;i<NUM_PLAT;i++) {
    dkR(DKP[i].x, DKP[i].y, DKP[i].w, 5, DKC_PLAT);
    for (int j=0;j<DKP[i].w-8;j+=20) {
      int sx = DKP[i].right ? DKP[i].x+j : DKP[i].x+DKP[i].w-j-8;
      dkR(sx, DKP[i].y-2, 6, 2, 0x528A);
    }
  }
  for (int i=0;i<NUM_LAD;i++) {
    int h=DKL[i].yBot-DKL[i].yTop;
    dkR(DKL[i].x, DKL[i].yTop, 8, h, DKC_LAD);
    for (int r=0;r<h;r+=8)
      dkR(DKL[i].x, DKL[i].yTop+r, 8, 2, 0xFFC0);
  }
  
  // Menggeser Princess Pauline ke posisi aman paling kanan lantai 4 (x=280)
  dkR(280, 26, 10, 16, DKC_PAUL);
  dkR(283, 19,  6,  8, DKC_MS);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.setCursor(265, DK_ARENA_TOP+10); tft.print("HELP!");
}

void dkEraseRect(int x, int y, int w, int h) {
  tft.fillRect(x, DK_ARENA_TOP + y, w, h, DKC_BG);
  for (int i = 0; i < NUM_PLAT; i++) {
    int interX1 = max(x, (int)DKP[i].x);
    int interY1 = max(y, (int)DKP[i].y);
    int interX2 = min(x + w, (int)(DKP[i].x + DKP[i].w));
    int interY2 = min(y + h, (int)(DKP[i].y + 5));
    if (interX2 > interX1 && interY2 > interY1) {
      tft.fillRect(interX1, DK_ARENA_TOP + interY1, interX2 - interX1, interY2 - interY1, DKC_PLAT);
    }
    for (int j = 0; j < DKP[i].w - 8; j += 20) {
      int sx = DKP[i].right ? DKP[i].x + j : DKP[i].x + DKP[i].w - j - 8;
      int sInterX1 = max(x, sx);
      int sInterY1 = max(y, (int)(DKP[i].y - 2));
      int sInterX2 = min(x + w, sx + 6);
      int sInterY2 = min(y + h, (int)DKP[i].y);
      if (sInterX2 > sInterX1 && sInterY2 > sInterY1) {
        tft.fillRect(sInterX1, DK_ARENA_TOP + sInterY1, sInterX2 - sInterX1, sInterY2 - sInterY1, 0x528A);
      }
    }
  }
  for (int i = 0; i < NUM_LAD; i++) {
    int ladH = DKL[i].yBot - DKL[i].yTop;
    int interX1 = max(x, (int)DKL[i].x);
    int interY1 = max(y, (int)DKL[i].yTop);
    int interX2 = min(x + w, (int)(DKL[i].x + 8));
    int interY2 = min(y + h, (int)DKL[i].yBot);
    if (interX2 > interX1 && interY2 > interY1) {
      tft.fillRect(interX1, DK_ARENA_TOP + interY1, interX2 - interX1, interY2 - interY1, DKC_LAD);
      for (int r = 0; r < ladH; r += 8) {
        int ry = DKL[i].yTop + r;
        int rInterX1 = max(x, (int)DKL[i].x);
        int rInterY1 = max(y, ry);
        int rInterX2 = min(x + w, (int)(DKL[i].x + 8));
        int rInterY2 = min(y + h, ry + 2);
        if (rInterX2 > rInterX1 && rInterY2 > rInterY1) {
          tft.fillRect(rInterX1, DK_ARENA_TOP + rInterY1, rInterX2 - rInterX1, rInterY2 - rInterY1, 0xFFC0);
        }
      }
    }
  }
}

void dkDrawDK(bool e) {
  int x=mkDK.x, y=mkDK.y;
  if (e) { dkEraseRect(x - 8, y, 48, 26); return; }
  uint16_t c=DKC_DK, cs=DKC_MS;
 
  dkR(x,   y+8, 28,18,c);
  dkR(x+4, y,   20,12,c);
  dkR(x+7, y+3,  4, 3,cs);
  dkR(x+17,y+3,  4, 3,cs);
  dkR(x-8, y+8, 10, 8,c);
  if (mkDK.throwing) {
    dkR(x+26,y+2,14, 6,DKC_DK);
    dkR(x+36,y,  12,10,DKC_BR);
  } else {
    dkR(x+26,y+8,10, 8,c);
  }
}

void dkDrawMario(bool e) {
  int x=(int)mkMario.x, y=(int)mkMario.y;
  if (e) { dkEraseRect(x - 1, y, MR_W + 2, MR_H); return; }
  uint16_t cb=DKC_MB, cs=DKC_MS;
  dkR(x+1, y+7, 10, 8,cb);
  dkR(x+1, y+1, 10, 7,cs);
  dkR(x,   y,   12, 3,cb);
  if (mkMario.onLadder) {
    dkR(x+1,y+15,4,5,cb); dkR(x+7,y+15,4,5,cs);
  } else if ((mkMario.anim&1)==0) {
    dkR(x+1,y+15,4,5,cb); dkR(x+7,y+15,4,5,cb);
  } else {
    dkR(x-1,y+15,5,5,cb); dkR(x+8,y+15,5,5,cb);
  }
}

void dkDrawBarrel(int i, bool e) {
  if (!mkBR[i].active) return;
  int x=(int)mkBR[i].x, y=(int)mkBR[i].y;
  if (e) { dkEraseRect(x, y, BR_W, BR_H); return; }
  uint16_t c=DKC_BR, cl=0xFFFF;
  dkR(x,y,BR_W,BR_H,c);
  int o=(mkBR[i].anim&1)*3;
  dkR(x+2+o,y+1,2,8,cl);
  if (x+7+o<x+BR_W) dkR(x+7+o,y+1,2,8,cl);
}

void dkHUD() {
  tft.fillRect(0,0,DK_W,DK_ARENA_TOP,ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE); tft.setTextSize(1);
  tft.setCursor(4,5);  tft.print("SC:"); tft.print(dkScore);
  tft.setCursor(90,5);  tft.print("LV:"); tft.print(dkLevel);
  tft.setCursor(130,5); tft.print("LIVES:");
  for (int i=0;i<min(dkLives,5);i++)
    tft.fillRect(180+i*14,3,10,12,DKC_MB);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(255,5); tft.print("[FIRE]=Menu");
}

// ── PHYSICS HELPERS ─────────────────────────────────────────
int dkStandOn(float x, float y, int w, int h) {
  float feet = y + h;
  for (int i=0;i<NUM_PLAT;i++) {
    float top = DKP[i].y;
    if (feet >= top-1 && feet <= top+4) {
      if (x+w > DKP[i].x && x < DKP[i].x+DKP[i].w) return i;
    }
  }
  return -1;
}

int dkLandOn(float x, float prevY, float newY, int w, int h) {
  float prevFeet = prevY + h;
  float newFeet  = newY  + h;
  for (int i=0;i<NUM_PLAT;i++) {
    float top = DKP[i].y;
    if (prevFeet <= top+1 && newFeet >= top) {
      if (x+w > DKP[i].x && x < DKP[i].x+DKP[i].w) return i;
    }
  }
  return -1;
}

int dkOnLadder(float x, float y) {
  int cx = (int)x + MR_W/2;
  int feetY = (int)y + MR_H;
  for (int i=0; i<NUM_LAD; i++) {
    if (abs(cx - (DKL[i].x + 4)) <= 10) {
      if (feetY >= DKL[i].yTop && (int)y <= DKL[i].yBot) return i;
    }
  }
  return -1;
}

int dkLadderBelow(float x, float y) {
  int cx = (int)x + MR_W/2;
  int fy = (int)(y + MR_H);
  for (int i=0; i<NUM_LAD; i++) {
    if (abs(cx - (DKL[i].x + 4)) <= 10) {
      if (fy >= DKL[i].yTop - 6 && fy <= DKL[i].yTop + 6) return i;
    }
  }
  return -1;
}

int dkBarrelLadder(int i) {
  int cx = (int)mkBR[i].x + BR_W/2;
  int cy = (int)mkBR[i].y + BR_H/2;
  for (int l=0; l<NUM_LAD; l++) {
    if (cx >= DKL[l].x - 4 && cx <= DKL[l].x + 12 && cy >= DKL[l].yTop && cy <= DKL[l].yBot) return l;
  }
  return -1;
}

void dkSpawnBarrel() {
  for (int i=0;i<MAX_BR;i++) {
    if (!mkBR[i].active) {
      mkBR[i].active   = true;
      mkBR[i].onLadder = false;
      mkBR[i].platIdx  = NUM_PLAT-1;
      mkBR[i].x  = mkDK.x + 10;
      mkBR[i].y  = DKP[NUM_PLAT-1].y - BR_H;
      mkBR[i].vx = DKP[NUM_PLAT-1].right ? 1.3f : -1.3f;
      mkBR[i].vy = 0;
      mkBR[i].anim = 0;
      mkBR[i].animT= millis();
      mkBR[i].lastLadderCheck = -1; 
      break;
    }
  }
}

void dkUpdateBarrel(int i) {
  if (!mkBR[i].active) return;
  dkDrawBarrel(i, true);
  if (millis()-mkBR[i].animT > 90) { mkBR[i].anim^=1; mkBR[i].animT=millis(); }

  float px=mkBR[i].x, py=mkBR[i].y;
  if (mkBR[i].onLadder) {
    mkBR[i].vy = 1.2f;
    mkBR[i].y += mkBR[i].vy;
    int land = dkLandOn(mkBR[i].x, py, mkBR[i].y, BR_W, BR_H);
    if (land >= 0 && land < mkBR[i].platIdx) {
      mkBR[i].y = DKP[land].y - BR_H;
      mkBR[i].vy = 0;
      mkBR[i].onLadder = false;
      mkBR[i].platIdx  = land;
      mkBR[i].vx = DKP[land].right ? 1.3f : -1.3f;
      mkBR[i].lastLadderCheck = -1;
    }
    if (mkBR[i].active) dkDrawBarrel(i,false);
    return;
  }

  mkBR[i].vy += 0.45f;
  mkBR[i].y  += mkBR[i].vy;
  if (dkStandOn(mkBR[i].x, py, BR_W, BR_H) >= 0) {
    mkBR[i].x  += mkBR[i].vx;
  }

  int land = dkLandOn(mkBR[i].x, py, mkBR[i].y, BR_W, BR_H);
  if (land>=0 && mkBR[i].vy>0) {
    mkBR[i].y  = DKP[land].y - BR_H;
    mkBR[i].vy = 0; 
    mkBR[i].platIdx = land;
    mkBR[i].vx = DKP[land].right ? 1.3f : -1.3f; 

    int cx = (int)mkBR[i].x + BR_W/2;
    int currentLadder = -1;
    for (int ll=0; ll<NUM_LAD; ll++) {
      if (abs(DKL[ll].yTop - DKP[land].y) < 8) {
        if (abs(cx - (DKL[ll].x + 4)) < 8) {
          currentLadder = ll;
          break;
        }
      }
    }

    if (currentLadder >= 0) {
      if (mkBR[i].lastLadderCheck != currentLadder) {
        mkBR[i].lastLadderCheck = currentLadder;
        if (random(0, 2) == 0) { 
          mkBR[i].x = DKL[currentLadder].x - BR_W/2 + 4;
          mkBR[i].onLadder = true;
          mkBR[i].vx = 0; 
          mkBR[i].vy = 0;
        }
      }
    } else {
      mkBR[i].lastLadderCheck = -1;
    }
  }

  if (!mkBR[i].onLadder && mkBR[i].vy==0) {
    if (dkStandOn(mkBR[i].x, mkBR[i].y, BR_W, BR_H)<0) mkBR[i].vy=0.1f;
  }

  if (mkBR[i].y > DK_H+10) { mkBR[i].active=false; dkScore+=10; return; }
  if (mkBR[i].x < -20 || mkBR[i].x > DK_W + 20) {
    if (mkBR[i].platIdx == 0) {
      mkBR[i].active = false;
      return;
    }
  }

  dkDrawBarrel(i,false);
}

bool dkHit() {
  for (int i=0;i<MAX_BR;i++) {
    if (!mkBR[i].active) continue;
    bool hx = mkBR[i].x+BR_W > mkMario.x+1 && mkBR[i].x < mkMario.x+MR_W-1;
    bool hy = mkBR[i].y+BR_H > mkMario.y+3 && mkBR[i].y < mkMario.y+MR_H-1;
    if (hx&&hy) return true;
  }
  return false;
}

void dkResetMario() {
  mkMario.x  = 8;
  mkMario.y  = DKP[0].y - MR_H;
  mkMario.vy = 0;
  mkMario.grounded = true;
  mkMario.onLadder = false;
  mkMario.jumping  = false;
  mkMario.anim = 0;
}

void dkDeath() {
  dkLives--;
  for (int f=0;f<8;f++) {
    dkDrawMario(true);
    uint16_t fc=(f%2)?ILI9341_YELLOW:ILI9341_RED;
    dkR((int)mkMario.x,(int)mkMario.y,MR_W,MR_H,fc);
    delay(70);
    dkR((int)mkMario.x,(int)mkMario.y,MR_W,MR_H,DKC_BG);
    delay(70);
  }
  
  if (dkLives<=0) { 
    dkGameOver=true; 
    tft.fillRect(55,70,210,80,DKC_BG);
    tft.drawRect(55,70,210,80,ILI9341_RED);
    tft.setTextColor(ILI9341_RED);    tft.setTextSize(2);
    tft.setCursor(106,80);
    tft.print("GAME OVER");
    tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(1);
    int scoreTextWidth = 42 + (dkScore == 0 ? 6 : (String(dkScore).length() * 6));
    tft.setCursor(55 + (210 - scoreTextWidth) / 2, 110); 
    tft.print("Score: "); tft.print(dkScore);
    tft.setCursor(118,125); tft.print("[UP] = Restart");
    return;
  }
  
  for (int i=0;i<MAX_BR;i++) { dkDrawBarrel(i,true); mkBR[i].active=false; }
  dkResetMario();
  dkDrawArena();
  dkDrawDK(false);
  dkHUD();
  dkDrawMario(false);
  dkBarrelTimer=millis();
}

void dkLevelUp() {
  dkLevel++; dkScore+=500;
  dkBarrelMs = max(1400, 2800-dkLevel*180);
  for (int f=0;f<5;f++) {
    tft.fillRect(0,DK_ARENA_TOP,DK_W,DK_H,ILI9341_WHITE); delay(90);
    tft.fillRect(0,DK_ARENA_TOP,DK_W,DK_H,DKC_BG);
    delay(90);
  }
  for (int i=0;i<MAX_BR;i++) mkBR[i].active=false;
  dkResetMario();
  dkDrawArena(); dkDrawDK(false);
  dkHUD(); dkDrawMario(false);
  dkBarrelTimer=millis();
}

void donkeyKongInit() {
  dkScore=0; dkLives=3; dkLevel=1;
  dkBarrelMs=1500; dkGameOver=false;
  for (int i=0;i<MAX_BR;i++) mkBR[i].active=false;
  dkResetMario();
  // Posisi Kong & mekanisme lempar asli dipertahankan penuh
  mkDK.x=100;
  mkDK.y=DKP[NUM_PLAT-1].y-26;
  mkDK.throwing=false; mkDK.throwTick=0; mkDK.anim=0; mkDK.animT=0;
  dkBarrelTimer=millis(); dkTick=millis();
  tft.fillScreen(ILI9341_BLACK);
  dkDrawArena(); dkDrawDK(false); dkHUD(); dkDrawMario(false);
}

void donkeyKongLoop() {
  if (btnFireEdge()) { returnToMenu(); return; }

  if (dkGameOver) {
    if (btnUpEdge()) donkeyKongInit();
    return;
  }

  if (millis()-dkTick < 18) return;
  dkTick=millis();

  // ── MARIO INPUT & PHYSICS ─────────────────────────────────
  dkDrawMario(true);
  float prevY = mkMario.y;
  if (!mkMario.onLadder) {
    float nx = mkMario.x;
    if (btnLeft())       nx -= MR_SPEED;
    else if (btnRight()) nx += MR_SPEED;
    nx = constrain(nx, 0, DK_W - MR_W);
    mkMario.x = nx;
    if (btnUpEdge() && mkMario.grounded && !mkMario.jumping) {
      mkMario.vy      = MR_JUMP;
      mkMario.jumping  = true;
      mkMario.grounded = false;
    }

    if (btnUp()) {
      int l = dkOnLadder(mkMario.x, mkMario.y);
      if (l>=0 && !mkMario.jumping) {
        // Hanya boleh memanjat JIKA posisi kaki Mario berada di bawah ujung atas tangga (+4 pixel toleransi)
        if ((int)mkMario.y + MR_H > DKL[l].yTop + 4) {
          mkMario.onLadder = true;
          mkMario.vy = 0;
          mkMario.x  = DKL[l].x - 2.0f;
        }
      }
    }
    
    if (btnDown() && mkMario.grounded) {
      int l = dkLadderBelow(mkMario.x, mkMario.y);
      if (l>=0) {
        mkMario.onLadder = true;
        mkMario.vy = 0;
        mkMario.y  = mkMario.y + 4; 
        mkMario.x  = DKL[l].x - 2.0f;
        mkMario.grounded = false;
      }
    }

    mkMario.vy += MR_GRAV;
    mkMario.y  += mkMario.vy;
    for (int i = 0; i < NUM_PLAT; i++) {
      if (mkMario.y >= DKP[i].y && mkMario.y <= DKP[i].y + 5) {
        if (mkMario.x + MR_W > DKP[i].x && mkMario.x < DKP[i].x + DKP[i].w) {
          if (mkMario.vy < 0) { 
            mkMario.y = DKP[i].y + 5;
            mkMario.vy = 0.5f;        
          }
        }
      }
    }

    mkMario.grounded = false;
    int land = dkLandOn(mkMario.x, prevY, mkMario.y, MR_W, MR_H);
    if (land>=0 && mkMario.vy>=0) {
      mkMario.y       = DKP[land].y - MR_H;
      mkMario.vy      = 0;
      mkMario.grounded = true;
      mkMario.jumping  = false;
    }

    if (!mkMario.grounded) {
      int st = dkStandOn(mkMario.x, mkMario.y, MR_W, MR_H);
      if (st>=0 && mkMario.vy>=0) {
        mkMario.y       = DKP[st].y - MR_H;
        mkMario.vy      = 0;
        mkMario.grounded = true;
        mkMario.jumping  = false;
      }
    }
  } else {
    int l = dkOnLadder(mkMario.x, mkMario.y);
    if (l<0) {
      mkMario.onLadder = false;
      mkMario.vy = 0;
    } else {
      if (btnUp()) {
        mkMario.vy = -MR_LAD_SPEED; // Menggunakan speed baru yang lebih lambat
        if (mkMario.y <= DKL[l].yTop - MR_H + 4) {
          mkMario.y        = DKL[l].yTop - MR_H + 2;
          mkMario.onLadder = false;
          mkMario.grounded = true;
          mkMario.vy       = 0;
        }
      } else if (btnDown()) {
        mkMario.vy = MR_LAD_SPEED;  // Menggunakan speed baru yang lebih lambat
        if (mkMario.y + MR_H >= DKL[l].yBot + 2) {
          mkMario.onLadder = false;
          mkMario.vy = 0;
        }
      } else {
        mkMario.vy = 0;
      }
      mkMario.y += mkMario.vy;
    }
  }

  if (mkMario.y + MR_H > DK_H) {
    mkMario.y = DK_H - MR_H;
    mkMario.vy = 0; mkMario.jumping = false; mkMario.grounded = true;
  }

  if (millis()-mkMario.animT > 130) { mkMario.anim^=1; mkMario.animT=millis(); }
  dkDrawMario(false);

  // ── HITBOX PRINCESS (VERSI FIX - WAJIB GROUNDED DI LANTAI 4) ──
  if (mkMario.x + MR_W > 280 && mkMario.x < 290) {
    if (mkMario.grounded && mkMario.y >= 20 && mkMario.y <= 24) {
      dkLevelUp();
      return; 
    }
  }

  // ── DK ANIMASI ────────────────────────────────────────────
  if (millis()-mkDK.animT > 320) {
    mkDK.anim^=1; mkDK.animT=millis();
    dkDrawDK(true); dkDrawDK(false);
  }
  if (mkDK.throwing) {
    mkDK.throwTick-=18;
    if (mkDK.throwTick<=0) {
      mkDK.throwing=false;
      dkDrawDK(true); dkDrawDK(false);
    }
  }

  // Spawn barrel
  if (millis()-dkBarrelTimer > (unsigned long)dkBarrelMs) {
    dkBarrelTimer=millis();
    mkDK.throwing=true; mkDK.throwTick=480;
    dkDrawDK(true); dkDrawDK(false);
    dkSpawnBarrel();
  }

  // ── BARREL UPDATE ─────────────────────────────────────────
  for (int i=0;i<MAX_BR;i++) dkUpdateBarrel(i);
  
  // ── HIT CHECK ─────────────────────────────────────────────
  if (dkHit()) dkDeath();

  static unsigned long hudT=0;
  if (millis()-hudT>700) { dkHUD(); hudT=millis(); }
}