#define SPEED 100
const uint8_t LEDMATRIX_CS_PIN = D1;

// Define LED Matrix dimensions (0-n) - eg: 32x8 = 31x7
const int LEDMATRIX_WIDTH = 31;  
const int LEDMATRIX_HEIGHT = 7;
const int LEDMATRIX_SEGMENTS = 4;

int x=-1, y=0;
bool s = true;

//Interval to ping server in seconds
int handlePingSecs = 120; // 2 minutes

int loopCnt = 0;
int pingCnt = 0;

String eyeMode;
bool animationActive = false;
int activeCnt = 0;
int animationPeriodSecs = 0;
int animationSecsFallback = 30;
int eyeMovement = 0;

char marqueeText[100];
