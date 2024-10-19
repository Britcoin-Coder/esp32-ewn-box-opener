#include <Arduino.h>
#define JSON_CONFIG_FILE "/apikey_config.json" // JSON file for api key
#define WIFI_PIN 0 // left button for wifi config
#define CHARGE_PIN 35 // right button to reset charge
#define NTP_SERVER     "pool.ntp.org" // server to get time from
#define UTC_OFFSET     0 //offset from UTC (no need to change as it isn't used for showing time)
#define UTC_OFFSET_DST 0 //offset for daylight saving (no need to change as it isn't used for showing time)


bool wifibuttonPressed = false; // initialise wifi button state
bool chargebuttonPressed = false; // initialise charge button state
bool shouldSaveConfig = false; // Flag for saving API data
bool showKitty = false; // show kitty sprite
bool showCharge = false; //show charge sprite

const char *apiUrl = "https://api.erwin.lol/"; //=====Box Opener client setup mainnet

char apiKey[300]; // Variable to hold api key from wifi setup

const int numGuesses = 50; //number of guesses per round

int sleepTime = 10000; // default sleep time in ms
int dataS = 0; //succesful data count
int dataF = 0; //failed data count
int dataT = 0; //Timeout count
int dataP = 0; //success percentage
int chargeCheck = 0; //count of consecutive rejects
int chargeTimer = 259080; //72 hour timer (minus 120 seconds)
int expireTime; //future time where charge expires
unsigned long epochTime; //time in epoch format
int text1Offset =0; //offset for text
  
String mnemonics[numGuesses]; // bip39 mnemonic table