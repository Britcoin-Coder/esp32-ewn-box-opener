#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite background= TFT_eSprite(&tft);
TFT_eSprite txtSprite1= TFT_eSprite(&tft);
TFT_eSprite txtSprite2= TFT_eSprite(&tft);
TFT_eSprite Kitty= TFT_eSprite(&tft);
TFT_eSprite Charge= TFT_eSprite(&tft);

int backgroundXSize = 135;
int backgroundYSize = 240;

String chooseSprite; //which sprite to show

int kittyXSize = 98;
int kittyYSize = 124;
int kittyXPos = 21;
int kittyYPos = 62;

int chargeXSize = 77;
int chargeYSize = 88;
int chargeXPos = 30;
int chargeYPos = 68;

uint16_t text1Color; //colour for text display
String text1String; //message for text display
int text1XSize = 100;
int text1YSize = 20;
int text1XPos = 20;
int text1YPos = 200;

int text2XSize = 100;
int text2YSize = 20;
int text2YPo2 = 20;

int meterXPos = 35;
int meterYPos = 15;
int meterWidth = 5;
int meterHeight = 10;
int meterRadius = 2;
// Define colors for each charge level
uint16_t colors[10] = {
TFT_RED, TFT_RED, 
TFT_ORANGE, TFT_ORANGE,
TFT_YELLOW, TFT_YELLOW,
TFT_GREENYELLOW,TFT_GREENYELLOW,
TFT_GREEN, TFT_GREEN
};