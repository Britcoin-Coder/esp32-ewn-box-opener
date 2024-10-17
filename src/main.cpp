/*
 * ------------------------------------------------------------------------
 * Project Name: esp32-ewn-box-opener
 * Description: Esp32 port of the Box Opener client for mining EWN tokens.
 * Author: Crey
 * Repository: https://github.com/cr3you/esp32-ewn-box-opener/
 * Fork Author: bigdaveakers
 * Fork Repository: https://github.com/Britcoin-Coder/esp32-ewn-box-opener
 * Date: 2024.10.16 
 * Version: 2.0
 * License: MIT
 * ------------------------------------------------------------------------
 */

#include <string>
#include "bip39/bip39.h"
#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <FS.h>
#include <TFT_eSPI.h> 
#include "box.h"
#include "kitty.h"
#include "charge.h"
#include <ESP32Time.h>

ESP32Time rtc(0);

TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite background= TFT_eSprite(&tft);
TFT_eSprite txtSprite1= TFT_eSprite(&tft);
TFT_eSprite txtSprite2= TFT_eSprite(&tft);
TFT_eSprite Kitty= TFT_eSprite(&tft);
TFT_eSprite Charge= TFT_eSprite(&tft);

WiFiManager wm;

#define JSON_CONFIG_FILE "/apikey_config.json" // JSON file for api key
#define WIFI_PIN 0 // left button for wifi config
#define CHARGE_PIN 35 // right button to reset charge

bool wifibuttonPressed = false; // initialise wifi button state
bool chargebuttonPressed = false; // initialise charge button state
bool shouldSaveConfig = false; // Flag for saving API data
bool showKitty = false; // show kitty sprite
bool showCharge = false; //show charge sprite

const char *apiUrl = "https://api.erwin.lol/"; //=====Box Opener client setup mainnet

char apiKey[300]; // Variable to hold api key from wifi setup

const int numGuesses = 50;

int sleepTime = 10000; // default sleep time in ms
int dataS = 0; //succesful data count
int dataF = 0; //failed data count
int dataT = 0; //Timeout count
int dataP = 0; //success percentage
int chargeCheck = 0; //count of consecutive rejects
int chargeTimer = 259200; //72 hour timer
int text1Offset =0; //offset for text
  
uint16_t text1Color;
String text1String;
String mnemonics[numGuesses]; // bip39 mnemonic table

//wifi button interrupt
void IRAM_ATTR handlewifiButtonPress()
{
  wifibuttonPressed = true;
}

//charge button interrupt
void IRAM_ATTR handlechargeButtonPress()
{
  chargebuttonPressed = true;
}

//charge level display
void renderCharge()
{
  int currentTime = rtc.getEpoch();   // Get the time once
  int remainingTime = chargeTimer - currentTime; // Calculate remaining time
  int chargeLevel = (remainingTime / (chargeTimer / 11)); // Determine current charge level (0-10)

  // Define colors for each charge level
  uint16_t colors[10] = {
    TFT_RED, TFT_RED, 
    TFT_ORANGE, TFT_ORANGE,
    TFT_YELLOW, TFT_YELLOW,
    TFT_GREENYELLOW,TFT_GREENYELLOW,
    TFT_GREEN, TFT_GREEN
  };

  // Loop through and draw rectangles based on the charge level
  for (int i = 0; i < chargeLevel; ++i)
  {
    int xPosition = 34 + (i * 7);  // Calculate x position of the rectangle
    tft.fillRoundRect(xPosition, 228, 5, 10, 2, colors[i]);  // Draw the rectangle
  }
}

// Save api key to spiffs
void saveConfigFile()
// Save Config in JSON format
{
    Serial.println(F("Saving configuration..."));
  
    // Create a JSON document
    StaticJsonDocument<512> json;
    Serial.println(apiKey);
    json["api_key"] = apiKey;
 
    // Open config file
    File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
    if (!configFile)
    {
        // Error, file did not open
        Serial.println("failed to open config file for writing");
    }
  
    // Serialize JSON data to write to file
    serializeJsonPretty(json, Serial);
    if (serializeJson(json, configFile) == 0)
    {
        // Error writing file
        Serial.println(F("Failed to write to file"));
    }

    // Close file
    configFile.close();
}

// Load api key from spiffs
bool loadConfigFile()
{
    // Load existing configuration file
    // Uncomment if we need to format filesystem
    // SPIFFS.format();

    // Read configuration from FS json
    Serial.println("Mounting File System...");

    // May need to make it begin(true) first time you are using SPIFFS
    if (SPIFFS.begin(false) || SPIFFS.begin(true))
    {
        Serial.println("mounted file system");
        if (SPIFFS.exists(JSON_CONFIG_FILE))
        {
            // The file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
            if (configFile)
            {
                Serial.println("Opened configuration file");
                StaticJsonDocument<512> json;
                DeserializationError error = deserializeJson(json, configFile);
                serializeJsonPretty(json, Serial);
                if (!error)
                {
                    Serial.println("Parsing JSON");

                    strcpy(apiKey, json["api_key"]);

                    return true;
                }
                else
                {
                    // Error loading JSON data
                    Serial.println("Failed to load json config");
                }
            }
        }
    }
    else
    {
        // Error mounting file system
        Serial.println("Failed to mount FS");
    }

    return false;
}

// Callback notifying us of the need to save api key
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

// Called when wifi config mode called
void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println("Entered Configuration Mode");

    Serial.print("Config SSID: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());

    Serial.print("Config IP Address: ");
    Serial.println(WiFi.softAPIP());
}

// Send data to display
void displayImage(TFT_eSprite &background, const String &message1, uint16_t text1Color, uint16_t bgColor, int xPos1, int yPos1
                  , const String &message2, bool isKitty, bool isCharge) {
    // Display empty box and Setup WiFi message
    background.pushImage(0, 0, 135, 240, box);
    txtSprite1.setTextColor(text1Color, bgColor);
    txtSprite1.fillSprite(bgColor);
    txtSprite1.drawString(message1, xPos1, 0, 2);
    txtSprite1.pushToSprite(&background, 20, yPos1, bgColor);
    txtSprite2.setTextColor(TFT_RED, bgColor);
    txtSprite2.fillSprite(bgColor);
    txtSprite2.drawString(message2, 0, 0, 2);
    txtSprite2.pushToSprite(&background, 18, 36, bgColor);
    if (isKitty == true)
    {  
      Kitty.pushImage(0, 0, 96, 133, kitty);
      Kitty.pushToSprite(&background, 26, 52, TFT_BLACK);
    }
    if (isCharge == true)
    {  
      Charge.pushImage(0,0,77,88,charge);
      Charge.pushToSprite(&background,30,68,TFT_BLACK);
    }

    // Render to screen
    background.pushSprite(0, 0);
    renderCharge();
}

// wifi and api key connection
void wificonnect()
{
    // Change to true when testing to force configuration every time we run
    bool forceConfig = false;

    bool spiffsSetup = loadConfigFile();
    if (!spiffsSetup)
    {
        Serial.println(F("Forcing config mode as there is no saved config"));
        forceConfig = true;
    }
    
    WiFi.mode(WIFI_STA);

    //wm.resetSettings();

    // Set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);

    // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wm.setAPCallback(configModeCallback);

    // Define a text box for the api key with 256 characters
    WiFiManagerParameter api_text("API", "Enter your API Key here", "default string", 256);

    // Add custom parameter for api key
    wm.addParameter(&api_text);

    Serial.print("Connecting to WiFi ..");

    // Display message note GREEN is RED
    displayImage(background, "Setup Wifi", TFT_GREEN, TFT_BLACK, 17, 205, "", showKitty, showCharge);

    bool res;

    res = wm.autoConnect("Erwin Box Opener", "password"); // password protected ap

    if (!res)
    {
        Serial.println("Failed to connect");

        // ESP.restart();
    }

    displayImage(background, "Connecting", TFT_WHITE, TFT_BLACK, 20, 205, "", showKitty, showCharge);

    delay(1000);

    // Save the custom parameters to FS
    if (shouldSaveConfig)
    {
        // Copy the string value
        strncpy(apiKey, api_text.getValue(), sizeof(apiKey));
        saveConfigFile();
    }
}

// Setup
void setup() {
  Serial.begin(115200);

  rtc.setTime(30);

  pinMode(WIFI_PIN, INPUT_PULLUP);
  attachInterrupt(WIFI_PIN, handlewifiButtonPress, RISING);

  pinMode(CHARGE_PIN, INPUT_PULLUP);
  attachInterrupt(CHARGE_PIN, handlechargeButtonPress, RISING);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

      // Create graphics elements
  background.createSprite(135, 240);
  background.setSwapBytes(true);
  Kitty.createSprite(96, 133);
  Charge.createSprite(77, 88);
  txtSprite1.createSprite(100, 20);
  txtSprite2.createSprite(100, 20);

  wificonnect();

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  Serial.println(WiFi.localIP()); // print local IP

  Serial.printf("===============\n");
  Serial.printf("Box-opener started\n");
}

// Generate table of 50 bip39 12-word mnemonics
void generateMnemonics(String *mnemonics) {
  for (int i = 0; i < numGuesses; i++) {
    std::vector<uint8_t> entropy(16); // 128-bit entropy -> 12 words
    for (size_t j = 0; j < entropy.size(); j++) {
      entropy[j] = esp_random() & 0xFF; // extract 1 random byte
    }
    BIP39::word_list mnemonic = BIP39::create_mnemonic(entropy);
    std::string mnemonicStr = mnemonic.to_string();
    mnemonics[i] = String(mnemonicStr.c_str());
    // Serial.printf("Generated mnemonic: %s\n", mnemonics[i].c_str());
  }
}

// Submit mnemonics to Oracle
bool submitGuesses(String *mnemonics, const String &apiUrl, const String &apiKey) {
  bool ret = false;
  DynamicJsonDocument jsonDoc(8192); // max size of the json output, to verify! (4kB was not enough)

   // Do some maths to work out the successful percent of guess rounds
  dataP = float(dataS) / float(dataS + dataF + dataT) * 100;
  String percent = String(dataP) + "%";


  for (int i = 0; i < numGuesses; i++) {
    jsonDoc.add(mnemonics[i]);
  }

  Serial.printf("🔑️ Generated %u guesses\n", numGuesses);
  Serial.printf("➡️ Submitting to oracle\n");



  // Check if charge is needed and setup kitty or charge image
  if (chargeCheck < 10) {
    showKitty = true;
    showCharge = false;
  }
  else
  {
    showKitty = false;
    showCharge = false;
  }

  displayImage(background, "Generating", TFT_WHITE, TFT_BLACK, 16, 205, percent, showKitty, showCharge);

  String jsonString;
  serializeJson(jsonDoc, jsonString);

  HTTPClient http;
  http.begin(apiUrl + "/submit_guesses");
  http.addHeader("x-api-key", apiKey);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000); // increase default 5s to 15s

  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0) {
    String response = http.getString();
    if (httpResponseCode == 202) {
      Serial.println("✅ Guesses accepted");

      // Setup Accepted message
      text1Color = TFT_BLUE; // Note BLUE is actually GREEN!
      text1String = "Accepted";
      text1Offset = 23;

      // Setup counts
      dataS++;
      chargeCheck = 0;
      ret = false;
    } else if (httpResponseCode == 404) { // "Closed Box Not Found"
      Serial.printf("❌ Guesses rejected (%d): %s\n", httpResponseCode, response.c_str());

      // Setup No Boxes message
      text1Color = TFT_GREEN; // Note GREEN is actually RED!
      text1String = "No Boxes";
      text1Offset = 25;

      // Setup counts
      dataF++;
      chargeCheck = 0; // if there is a valid response then the box doesn't need charging
      ret = false;
    } else { // other errors
      Serial.printf("❌ Guesses rejected (%d): %s\n", httpResponseCode, response.c_str());

      // Setup Rejected message
      text1Color = TFT_GREEN; // Note GREEN is actually RED
      text1String = "Rejected";
      text1Offset = 24;

      // Setup counts
      dataF++;
      chargeCheck++; // if a guess is rejected it COULD be due to needing to charge
      ret = true;
    }
  } else { // even more other errors :V maybe do a reconnect?
    Serial.printf("❌ Error in HTTP request: %s\n", http.errorToString(httpResponseCode).c_str());

    // Setup Timeout message
    text1Color = TFT_GREEN; // Note GREEN is actually RED
    text1String = "Timeout";
    text1Offset = 27;

    // Setup counts
    dataT++;
    ret = true;
  }

  http.end();

  // If there are 10 consecutive rejects without an accept, it is likely that the box needs charging
  if (chargeCheck >= 10) {
    // Setup CHARGE ME message and charge image
    text1Color = TFT_GREEN; // Note GREEN is actually RED
    text1String = "CHARGE ME";
    text1Offset = 18;
    showKitty = false;
    showCharge = true;
  } else { // if there is no charge needed, setup the kitty image
    showKitty = true;
    showCharge = false;
  }

 
  displayImage(background, text1String, text1Color, TFT_BLACK, text1Offset, 205, percent, showKitty, showCharge);

  return ret;
}

// Main loop
void loop() {
    // Is configuration portal requested?
    if (wifibuttonPressed) {
        wm.resetSettings();
        wificonnect();
        wifibuttonPressed = false;
    }

    // Is charge reset requested?
    if (chargebuttonPressed) {
        rtc.setTime(30);
        chargebuttonPressed = false;
    }

    //--- Reconnect WiFi if it is not connected for some reason
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("WiFi disconnected, trying to reconnect..\n");
        WiFi.disconnect();
        WiFi.reconnect();
    }

    Serial.println("⚙️ Generating guesses...");

    generateMnemonics(mnemonics);

    bool rateLimited = submitGuesses(mnemonics, apiUrl, (const char*)apiKey);

    if (rateLimited) {
        sleepTime += 10000;
    } else {
        sleepTime -= 1000;
    }

    if (sleepTime < 10000) {
        sleepTime = 10000;
    }
    
    if (sleepTime > 60000) { // If sleep for more than a minute, limit it to one minute
        sleepTime = 60000;
    }

    Serial.printf("waiting %is for next batch...\n", sleepTime / 1000);
    delay(sleepTime);
}
