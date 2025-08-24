#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS    5
#define TFT_RST   4
#define TFT_DC    2
#define TFT_MOSI  23
#define TFT_CLK   18

#define VRX_PIN 34
#define VRY_PIN 35
#define SW_PIN  32
#define BTN_PIN 14

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

const int CENTER = 2048;
const int DEADZONE = 500;

enum Scene { START, INGAME, END };
volatile Scene scene = START;
volatile bool buttonPressedFlag = false;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

int playerX, playerY;
const int playerSize = 10;
const int moveStep = 5;

int itemX, itemY;
const int itemSize = 10;

const int maxEnemies = 10;
int enemyX[maxEnemies], enemyY[maxEnemies];
int enemyDX[maxEnemies], enemyDY[maxEnemies];
bool enemyActive[maxEnemies];
const int enemySize = 12;
const int enemySpeed = 2;

unsigned long lastDirChange[maxEnemies];
const unsigned long dirChangeInterval = 5000; // 5 วินาที

int score = 0;
int currentEnemyCount = 0;

void IRAM_ATTR handleButtonPress() {
  unsigned long now = millis();
  if (now - lastDebounceTime > debounceDelay) {
    buttonPressedFlag = true;
    lastDebounceTime = now;
  }
}

String getJoystickDirection() {
  int xValue = analogRead(VRX_PIN);
  int yValue = analogRead(VRY_PIN);
  
  String dirX = "";
  String dirY = "";

  if (xValue < CENTER - DEADZONE) dirX = "R"; 
  else if (xValue > CENTER + DEADZONE) dirX = "L"; 

  if (yValue < CENTER - DEADZONE) dirY = "D"; 
  else if (yValue > CENTER + DEADZONE) dirY = "U"; 

  if (dirX == "" && dirY == "") return "C"; 
  else return dirX + dirY;
}

void spawnItem() {
  itemX = random(0, tft.width() - itemSize);
  itemY = random(0, tft.height() - itemSize);
  tft.fillRect(itemX, itemY, itemSize, itemSize, ILI9341_YELLOW);
}

void spawnEnemy(int index) {
  enemyX[index] = tft.width()/2;
  enemyY[index] = tft.height()/2;

  // สุ่มทิศทางจาก 8 ทิศ
  int dir = random(0, 8);
  int dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
  int dy[] = {0, 0, 1, -1, 1, -1, 1, -1};

  enemyDX[index] = dx[dir] * enemySpeed;
  enemyDY[index] = dy[dir] * enemySpeed;
  enemyActive[index] = true;
  lastDirChange[index] = millis();

  tft.fillRect(enemyX[index], enemyY[index], enemySize, enemySize, ILI9341_RED);
}

void changeEnemyDirection(int index) {
  int dir = random(0, 8);
  int dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
  int dy[] = {0, 0, 1, -1, 1, -1, 1, -1};
  enemyDX[index] = dx[dir] * enemySpeed;
  enemyDY[index] = dy[dir] * enemySpeed;
}

void drawStartScene() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 80);
  tft.println("Welcome2MyGame");
  tft.setTextSize(1);
  tft.setCursor(60, 140);
  tft.println("Pressed to start");
}

void drawIngameScene() {
  tft.fillScreen(ILI9341_BLACK);
  playerX = tft.width()/2;
  playerY = tft.height()/2;
  score = 0;
  currentEnemyCount = 0;

  for (int i = 0; i < maxEnemies; i++) {
    enemyActive[i] = false;
  }

  tft.fillRect(playerX, playerY, playerSize, playerSize, ILI9341_GREEN);
  spawnItem();
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.print("Score: ");
  tft.println(score);
}

void drawEndScene() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(50, 60);
  tft.println("Game Over");
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(1);
  tft.setCursor(80, 120);
  tft.print("Score : ");
  tft.println(score);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(50, tft.height() - 20);
  tft.println("By Nkxr V1.0.0");
}

void checkItemCollision() {
  if (playerX < itemX + itemSize &&
      playerX + playerSize > itemX &&
      playerY < itemY + itemSize &&
      playerY + playerSize > itemY) {
    score++;
    tft.fillRect(itemX, itemY, itemSize, itemSize, ILI9341_BLACK);
    spawnItem();
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setCursor(5, 5);
    tft.print("Score: ");
    tft.println(score);

    // เกิดศัตรูใหม่ตามเงื่อนไข
    if ((score == 1) || (score > 1 && score % 5 == 0)) {
      if (currentEnemyCount < maxEnemies) {
        spawnEnemy(currentEnemyCount);
        currentEnemyCount++;
      }
    }
  }
}

bool checkEnemyCollision() {
  for (int i = 0; i < currentEnemyCount; i++) {
    if (enemyActive[i]) {
      if (playerX < enemyX[i] + enemySize &&
          playerX + playerSize > enemyX[i] &&
          playerY < enemyY[i] + enemySize &&
          playerY + playerSize > enemyY[i]) {
        return true;
      }
    }
  }
  return false;
}

void updatePlayer() {
  String dir = getJoystickDirection();
  tft.fillRect(playerX, playerY, playerSize, playerSize, ILI9341_BLACK);

  if (dir.indexOf("L") >= 0) playerX -= moveStep;
  if (dir.indexOf("R") >= 0) playerX += moveStep;
  if (dir.indexOf("U") >= 0) playerY -= moveStep;
  if (dir.indexOf("D") >= 0) playerY += moveStep;

  if (playerX < 0) playerX = 0;
  if (playerX > tft.width() - playerSize) playerX = tft.width() - playerSize;
  if (playerY < 0) playerY = 0;
  if (playerY > tft.height() - playerSize) playerY = tft.height() - playerSize;

  tft.fillRect(playerX, playerY, playerSize, playerSize, ILI9341_GREEN);
  checkItemCollision();
}

void updateEnemies() {
  for (int i = 0; i < currentEnemyCount; i++) {
    if (enemyActive[i]) {
      tft.fillRect(enemyX[i], enemyY[i], enemySize, enemySize, ILI9341_BLACK);

      // เปลี่ยนทิศทุก 5 วินาที
      if (millis() - lastDirChange[i] >= dirChangeInterval) {
        changeEnemyDirection(i);
        lastDirChange[i] = millis();
      }

      // เคลื่อนที่
      enemyX[i] += enemyDX[i];
      enemyY[i] += enemyDY[i];

      // ชนขอบเด้งกลับ
      if (enemyX[i] < 0 || enemyX[i] > tft.width() - enemySize) {
        enemyDX[i] = -enemyDX[i];
        enemyX[i] += enemyDX[i];
      }
      if (enemyY[i] < 0 || enemyY[i] > tft.height() - enemySize) {
        enemyDY[i] = -enemyDY[i];
        enemyY[i] += enemyDY[i];
      }

      tft.fillRect(enemyX[i], enemyY[i], enemySize, enemySize, ILI9341_RED);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), handleButtonPress, FALLING);
  tft.begin();
  tft.setRotation(0);
  drawStartScene();
}

void loop() {
  if (buttonPressedFlag) {
    buttonPressedFlag = false;
    if (scene == START) scene = INGAME;
    else if (scene == INGAME) scene = END;
    else scene = START;

    if (scene == START) drawStartScene();
    else if (scene == INGAME) drawIngameScene();
    else if (scene == END) drawEndScene();
  }

  if (scene == INGAME) {
    updatePlayer();
    updateEnemies();
    if (checkEnemyCollision()) {
      scene = END;
      drawEndScene();
    }
  }
  delay(50);
}
