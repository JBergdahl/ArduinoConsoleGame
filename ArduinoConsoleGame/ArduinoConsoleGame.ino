const int potentiometerInPin = A0;
const int rightButtonPin = 2;
const int leftButtonPin = 3;
const int buzzPin = 9;

int potentiometerValue = 0;

int rightButtonState = 0;
int rightButtonLastState = 0;

int leftButtonState = 0;
int leftButtonLastState = 0;

bool isGameRunning = false;
bool isRightButtonPressed = false;
bool isLeftButtonPressed = false;
bool isScreenCleared = false;
bool isSelectArrowTop = true;
bool mainMenu = true;
bool inscreaseDifficulty = false;
bool isBackgroundRendered = false;
int roundCounter = 0;
int screenWidth = 0;
int screenHeight = 0;

int playerWidth = 20;
int playerHeight = 20;
int playerStartPositionX = 30;
int playerStartPositionY = 0;
int playerMostXPosition = playerStartPositionX + playerWidth;;
int playerLeastXPosition = playerStartPositionX;
int playerMostYPosition = 0;
int playerLeastYPosition = 0;
int playerYOffset = 0;
bool isJumping = true;
unsigned long lastJumpAnimationTime = 0;
unsigned long jumpAnimationDelay = 0;

uint16_t obstacleColor;
int obstacleWidth = 18;
int obstacleHeight = 18;
int obstacleLeastXPosition = 0;
int obstacleMostXPosition = 0;
int obstacleLeastYPosition = 0;
int obstaclePositionY = 0;
int firstObstacleXPosition = 0;
int obstacleXOffset = 0;
int obstacleCollisionPositionOffset = 0;
int objectsPassedPlayer = 0;
int maxObstaclesInRound = 3;
int obstacleCounter = 1;
int distanceBetweenObstacles = 100;
unsigned long lastObstacleAnimationTime = 0;
unsigned long ObstacleAnimationDelay = 10;

int oldScore = 0;
int newScore = 0;
int outputValueScoreMultiplier = 1;
char scoreString[12];
unsigned long lastScoreAnimationTime = 0;
unsigned long scoreAnimationDelay = 1000;

#include <Wire.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#define TFT_CS        10
#define TFT_RST        9
#define TFT_DC         8
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  pinMode(rightButtonPin, INPUT);
  pinMode(leftButtonPin, INPUT);

  // Init ST7789 240x135
  tft.init(135, 240);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(tft.getRotation() - 1);

  screenWidth = tft.width();
  screenHeight = tft.height();
  playerStartPositionY = screenHeight - 50;
  obstaclePositionY = screenHeight - 48;
  obstacleLeastYPosition = obstaclePositionY + obstacleHeight;
  obstacleColor = getObstacleColor();

  Serial.begin(9600);
  randomSeed(analogRead(0));

  //resetHighScoreInMemory();
}

void loop() {
  playerInput();
  if (isGameRunning) {
    gameLoop();
  } else {
    gameMenu();
  }
}

void gameLoop() {
  potentiometerValue = analogRead(potentiometerInPin);
  ObstacleAnimationDelay = map(potentiometerValue, 0, 1023, 7, 15);
  outputValueScoreMultiplier = map(potentiometerValue, 0, 1023, 3, 0);
  background();
  obstacleLoop();
  updateAndShowCurrentGameScore();
  if (isRightButtonPressed) {
    playerJump();
  }
  if (isLeftButtonPressed) {
    resetGameVariables();
  }
  collisionCheck();
}

void collisionCheck() {
  if (obstacleLeastXPosition <= playerMostXPosition) {
    if (obstacleLeastXPosition >= playerLeastXPosition) {
      if (obstacleLeastYPosition <= playerMostYPosition) {
        gameOver();
        return;
      }
    }
  }

  if (obstacleMostXPosition <= playerMostXPosition) {
    if (obstacleMostXPosition >= playerLeastXPosition) {
      if (obstacleLeastYPosition <= playerMostYPosition) {
        gameOver();
      }
    }
  }
}

void gameOver() {
  notifyScore();
  if (newScore > getHighScoreFromMemory()) {
    setHighScoreInMemory(newScore);
    playNewHighscoreSound();
  } else {
    playPlayerDeathSound();
  }
  resetGameVariables();
  delay(3000);
}

void notifyScore() {
  clearScreen();
  tft.setTextColor(ST77XX_WHITE);
  if (newScore > getHighScoreFromMemory()) {
    setCursorAndPrintToScreen(40, 40, "New highscore!");
  } else {
    setCursorAndPrintToScreen(90, 40, "Score:");
  }
  tft.setTextColor(ST77XX_YELLOW);
  sprintf(scoreString, "%05d", newScore);
  setCursorAndPrintToScreen(92, 70, scoreString);
}

void updateAndShowCurrentGameScore() {
  if ((millis() - lastScoreAnimationTime) > scoreAnimationDelay) {
    lastScoreAnimationTime = millis();

    tft.setTextColor(ST77XX_CYAN);
    sprintf(scoreString, "%d", oldScore);
    setCursorAndPrintToScreen(10, 10, scoreString);

    newScore += 10 * outputValueScoreMultiplier;
    oldScore = newScore;

    tft.setTextColor(ST77XX_YELLOW);
    sprintf(scoreString, "%d", newScore);
    setCursorAndPrintToScreen(10, 10, scoreString);
  }
}

void resetGameVariables() {
  isRightButtonPressed = false;
  isLeftButtonPressed = false;
  isGameRunning = false;
  isScreenCleared = false;
  obstacleXOffset = 0;
  playerYOffset = 0;
  isBackgroundRendered = false;
  playerStartPositionX = 30;
  playerWidth = 20;
  playerHeight = 20;
  inscreaseDifficulty = false;
  roundCounter = 0;
  obstacleCounter = 1;
  objectsPassedPlayer = 0;
  distanceBetweenObstacles = 100;
  maxObstaclesInRound = 3;
  oldScore = 0;
  newScore = 0;
}

void background() {
  if (!isBackgroundRendered) {
    tft.fillRect(0, 0, screenWidth, screenHeight - 30, ST77XX_CYAN); // Sky
    tft.fillRect(0, screenHeight - 30, screenWidth, 20, ST77XX_WHITE); // Snow
    tft.fillRect(0, screenHeight - 20, screenWidth, 20, ST77XX_ORANGE); // Ground
    tft.fillCircle(screenWidth - 70, 30, 15, ST77XX_YELLOW); // Sun
    
    //Clouds
    drawClouds();

    // Player
    tft.fillRect(playerStartPositionX, playerStartPositionY, 20, 20, ST77XX_YELLOW);
    playerMostYPosition = playerStartPositionY - playerYOffset + playerHeight;

    isBackgroundRendered = true;
  }
}

void drawClouds() {
  tft.fillCircle(screenWidth - 35, 40, 7, ST77XX_WHITE);
  tft.fillCircle(screenWidth - 50, 35, 12, ST77XX_WHITE);
  tft.fillCircle(screenWidth - 65, 40, 7, ST77XX_WHITE);

  tft.fillCircle(65, 50, 7, ST77XX_WHITE);
  tft.fillCircle(80, 45, 12, ST77XX_WHITE);
  tft.fillCircle(95, 50, 7, ST77XX_WHITE);
}

void playerJump() {

  playJumpSound();

  if ((millis() - lastJumpAnimationTime) > jumpAnimationDelay) {
    lastJumpAnimationTime = millis();
    calculatePlayerPosition();
    playerLeastYPosition = playerStartPositionY - playerYOffset;
    playerMostYPosition = playerLeastYPosition + playerHeight;
    drawPlayerModel();
    removeTrailingPlayerPixels();
  }
}

void calculatePlayerPosition() {
  if (isJumping) {
    playerYOffset++;
    if (playerYOffset % 10 == 0) {
      jumpAnimationDelay += 1;
    }
    if (playerYOffset <= 30) {
      if (playerYOffset % 5 == 0) {
        playerWidth--;
        playerHeight++;
        if (playerYOffset % 10 == 0) {
          playerStartPositionX++;
        }
      }
    }
    if (playerYOffset > 30) {
      if (playerYOffset % 5 == 0) {
        playerWidth++;
        playerHeight--;
        if (playerYOffset % 10 == 0) {
          playerStartPositionX--;
        }
      }
    }

    if ( playerYOffset == 59) {
      jumpAnimationDelay += 100;
    }
    if (playerYOffset >= 60) {
      isJumping = false;
    }
  } else {
    playerYOffset--;
    if (playerYOffset == 58) {
      jumpAnimationDelay -= 100;
    }
    if (playerYOffset % 10 == 0) {
      jumpAnimationDelay -= 1;
    }
    if (playerYOffset <= 0) {
      isJumping = true;
      isRightButtonPressed = false;
    }
  }
}

void drawPlayerModel() {
  if (isJumping) {
    tft.fillRect(playerStartPositionX, playerLeastYPosition, playerWidth, playerHeight, ST77XX_YELLOW);
  } else {
    tft.drawFastHLine(playerStartPositionX, playerMostYPosition, playerWidth, ST77XX_YELLOW);
  }
}

void removeTrailingPlayerPixels() {
  if (isJumping && playerYOffset != 0) {
    tft.drawFastHLine(playerStartPositionX, playerMostYPosition, playerWidth, ST77XX_CYAN);
    tft.drawFastHLine(playerStartPositionX - 5, playerMostYPosition - 1, playerWidth + 10, ST77XX_CYAN);
  } else {
    tft.drawFastHLine(playerStartPositionX, playerLeastYPosition, playerWidth, ST77XX_CYAN);
  }
}

void obstacleLoop() {
  if ((millis() - lastObstacleAnimationTime) > ObstacleAnimationDelay) {
    lastObstacleAnimationTime = millis();
    obstacleXOffset++;
  }
  startNewRoundIfAllObjectsHavePassed();
  firstObstacleXPosition = screenWidth - obstacleXOffset;
  moveObstaclesForward();
  removeTrailingObstaclePixels();
  updateCollision();
}

void startNewRoundIfAllObjectsHavePassed() {
  if (obstacleXOffset >= (screenWidth + 30 + distanceBetweenObstacles * (obstacleCounter - 1))) {
    obstacleColor = getObstacleColor();
    obstacleXOffset = 0;
    objectsPassedPlayer = 0;
    roundCounter++;
    if (roundCounter >= 2) {
      obstacleCounter = random(1, maxObstaclesInRound + 1);
      if (inscreaseDifficulty) {
        maxObstaclesInRound++;
        if (distanceBetweenObstacles > 80) {
          distanceBetweenObstacles -= 5;
        }
        inscreaseDifficulty = false;
      } else if (roundCounter % 5 == 0) {
        inscreaseDifficulty = true;
      }
    }
  }
}

uint16_t getObstacleColor() {
  int randomNumber = random(1, 8);
  switch (randomNumber) {
    case 1:
      obstacleColor = ST77XX_RED;
      break;
    case 2:
      obstacleColor = ST77XX_BLACK;
      break;
    case 3:
      obstacleColor = ST77XX_GREEN;
      break;
    case 4:
      obstacleColor = ST7735_ORANGE;
      break;
    case 5:
      obstacleColor = ST7735_BLUE;
      break;
    case 6:
      obstacleColor = ST7735_MAGENTA;
      break;
    case 7:
      obstacleColor = ST7735_MAGENTA;
      break;
    default:
      obstacleColor = ST77XX_RED;
  }
}

void moveObstaclesForward() {
  for (int i = 0; i <= obstacleCounter - 1; i++) {
    tft.drawFastVLine(firstObstacleXPosition + (distanceBetweenObstacles * i), obstaclePositionY, obstacleHeight, obstacleColor);
  }
}

void removeTrailingObstaclePixels() {
  for (int i = 0; i <= obstacleCounter - 1; i++) {
    tft.drawFastVLine(firstObstacleXPosition + obstacleWidth + (distanceBetweenObstacles * i), obstaclePositionY, obstacleHeight, ST77XX_CYAN);
  }
}

void updateCollision() {
  // Will only check obstacle closest to player for collision
  // When it is off screen, check next obstacle
  if (obstacleXOffset < (screenWidth + obstacleWidth + distanceBetweenObstacles * objectsPassedPlayer)) {
    obstacleCollisionPositionOffset = distanceBetweenObstacles * objectsPassedPlayer;
  } else {
    objectsPassedPlayer++;
  }

  obstacleLeastXPosition = firstObstacleXPosition + obstacleCollisionPositionOffset;
  obstacleMostXPosition = firstObstacleXPosition + obstacleWidth + obstacleCollisionPositionOffset;
}

void playerInput() {
  rightButtonState = digitalRead(rightButtonPin);
  leftButtonState = digitalRead(leftButtonPin);

  if (rightButtonLastState != rightButtonState) {
    rightButtonLastState = rightButtonState;
    if (rightButtonState == LOW) {
      Serial.println("right");
      isRightButtonPressed = true;
    }
  }

  if (leftButtonLastState != leftButtonState) {
    leftButtonLastState = leftButtonState;
    if (leftButtonState == LOW) {
      Serial.println("left");
      isLeftButtonPressed = true;
    }
  }
}

void gameMenu() {
  if (!isScreenCleared) {
    clearScreen();
    isScreenCleared = true;
    tft.drawRect(5, 5, screenWidth - 10, tft.height() - 10, ST77XX_WHITE);
    tft.drawRect(10, 10, screenWidth - 20, tft.height() - 20, ST77XX_WHITE);
  }

  if (isRightButtonPressed || isLeftButtonPressed) {
    playMenuArrowChangePositionSound();
  }

  if (mainMenu) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    setCursorAndPrintToScreen(75, 40, "New Game");
    setCursorAndPrintToScreen(67, 70, "Highscore");

    int highScoreFromMemory = getHighScoreFromMemory();
    sprintf(scoreString, "%05d%", highScoreFromMemory);
    tft.setTextColor(ST77XX_YELLOW);
    setCursorAndPrintToScreen(92, 95, scoreString);

    if (isSelectArrowTop) {
      tft.setTextColor(ST77XX_WHITE);
      setCursorAndPrintToScreen(35, 40, "->");

      tft.setTextColor(ST77XX_BLACK);
      setCursorAndPrintToScreen(35, 70, "->");
    } else {
      tft.setTextColor(ST77XX_WHITE);
      setCursorAndPrintToScreen(35, 70, "->");

      tft.setTextColor(ST77XX_BLACK);
      setCursorAndPrintToScreen(35, 40, "->");
    }

    if (isRightButtonPressed) {
      if (isSelectArrowTop) {
        isGameRunning = true;
        clearScreen();
      } else {
        isSelectArrowTop = true;
        mainMenu = false;
        isScreenCleared = false;
      }
      isRightButtonPressed = false;
    }

    if (isLeftButtonPressed) {
      isSelectArrowTop = isSelectArrowTop ? false : true;
      isLeftButtonPressed = false;
    }
  } else {
    clearHighscoreMenu();
  }
}

void clearHighscoreMenu() {
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  setCursorAndPrintToScreen(67, 20, "Clear the");
  setCursorAndPrintToScreen(67, 40, "highscore?");
  setCursorAndPrintToScreen(screenWidth / 2 - 10, 70, "NO");
  setCursorAndPrintToScreen(screenWidth / 2 - 17, 90, "YES");

  if (isSelectArrowTop) {
    tft.setTextColor(ST77XX_WHITE);
    setCursorAndPrintToScreen(screenWidth / 2 - 50, 70, "->");
    tft.setTextColor(ST77XX_BLACK);
    setCursorAndPrintToScreen(screenWidth / 2 - 50, 90, "->");
  } else {
    tft.setTextColor(ST77XX_WHITE);
    setCursorAndPrintToScreen(screenWidth / 2 - 50, 90, "->");
    tft.setTextColor(ST77XX_BLACK);
    setCursorAndPrintToScreen(screenWidth / 2 - 50, 70, "->");
  }

  if (isRightButtonPressed) {
    if (isSelectArrowTop) {
      mainMenu = true;
      isScreenCleared = false;
    } else {
      resetHighScoreInMemory();
      mainMenu = true;
      isScreenCleared = false;
    }
    isRightButtonPressed = false;
  }

  if (isLeftButtonPressed) {
    isSelectArrowTop = isSelectArrowTop ? false : true;
    isLeftButtonPressed = false;
  }
}

void clearScreen() {
  tft.fillScreen(ST77XX_BLACK);
}

void playMenuArrowChangePositionSound() {
  tone(buzzPin, 500, 50);
  delay(50);
  noTone(buzzPin);
}

void playJumpSound() {
  if (isGameRunning) {
    if (isJumping) {
      if (playerYOffset == 1)tone(buzzPin, 200, 50);
      if (playerYOffset == 20)tone(buzzPin, 300, 50);
      if (playerYOffset == 40)tone(buzzPin, 400, 50);
    }

    if (!isJumping) {
      if (playerYOffset == 60)tone(buzzPin, 400, 50);
      if (playerYOffset == 40)tone(buzzPin, 300, 50);
      if (playerYOffset == 20)tone(buzzPin, 200, 50);
    }
    if (playerYOffset == 0)noTone(buzzPin);
  }
}

void playPlayerDeathSound() {
  tone(buzzPin, 300, 200);
  delay(200);
  tone(buzzPin, 200, 200);
  delay(200);
  tone(buzzPin, 100, 200);
  delay(200);
  noTone(buzzPin);
}

void playNewHighscoreSound() {
  tone(buzzPin, 100, 200);
  delay(200);
  tone(buzzPin, 200, 200);
  delay(200);
  tone(buzzPin, 300, 200);
  delay(200);
  tone(buzzPin, 400, 200);
  delay(200);
  noTone(buzzPin);
}

void setCursorAndPrintToScreen(int cursorRowPosition, int cursorColPosition, const char* text) {
  tft.setCursor(cursorRowPosition, cursorColPosition);
  tft.print(text);
}

int getHighScoreFromMemory() {
  Wire.begin();
  Wire.beginTransmission(0x57);
  Wire.write(0);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(0x57, 5);
  String  bytesToString;
  while (Wire.available()) {
    char c = Wire.read();
    bytesToString.concat(c);
  }
  int highScoreFromMemory = atoi(bytesToString.c_str());
  return highScoreFromMemory;
}

void setHighScoreInMemory(int newHighScore) {
  Wire.begin();
  delay(100);
  Wire.beginTransmission(0x57);
  Wire.write(0x0);
  Wire.write(0x0);

  sprintf(scoreString, "%05d%", newHighScore);
  Wire.write(scoreString);

  byte error = Wire.endTransmission();
  delay(100);
  Wire.endTransmission(0x57);
}

void resetHighScoreInMemory() {
  Wire.begin();
  delay(100);
  Wire.beginTransmission(0x57);
  Wire.write(0x0);
  Wire.write(0x0);

  Wire.write("00000");

  byte error = Wire.endTransmission();
  Serial.print("status: ");
  Serial.println(error);
  delay(100);

  Wire.endTransmission(0x57);
}
