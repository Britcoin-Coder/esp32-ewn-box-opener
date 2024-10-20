/*
 * ------------------------------------------------------------------------
 * Project Name: esp32-ewn-box-opener
 * Description: Esp32 port of the Box Opener client for mining EWN tokens.
 * Author: Crey
 * Repository: https://github.com/cr3you/esp32-ewn-box-opener/
 * Fork Author: bigdaveakers
 * Fork Repository: https://github.com/Britcoin-Coder/esp32-ewn-box-opener
 * Date: 2024.10.16 
 * Version: 3.0
 * License: MIT
 * ------------------------------------------------------------------------
 */

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
#include "kittyhappy.h"
#include "kittyangry.h"
#include "charge.h"
#include "wifisetup.h"
#include "wifisymbol.h"
#include <ESP32Time.h>
#include <Preferences.h>
#include "screenconfig.h"
#include "ewnconfig.h"

ESP32Time rtc(0);

Preferences remainingTime;

WiFiManager wm;

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
  int remainingTime = expireTime - currentTime; // Calculate remaining time
  int chargeLevel = (remainingTime / (chargeTimer / 11)); // Determine current charge level (0-10)

  // Loop through and draw rectangles based on the charge level
  for (int i = 0; i < chargeLevel; ++i)
  {
    int xPosition = meterXPos + (i * 7);  // Calculate x position of the rectangle
    tft.fillRoundRect(xPosition, meterYPos, meterWidth, meterHeight, meterRadius, colors[i]);  // Draw the rectangle
  }
}

// Save api key to spiffs
void saveConfigFile()
// Save Config in JSON format
{
    Serial.println(F("Saving configuration..."));
  
    // Create a JSON document
    StaticJsonDocument<512> json;
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
                  , const String &message2, const String &overSprite) {
    // Display empty box and Setup WiFi message
    background.pushImage(0, 0, 135, 240, box);
    txtSprite1.setTextColor(text1Color, bgColor);
    txtSprite1.fillSprite(bgColor);
    txtSprite1.drawString(message1, xPos1, 0, 2);
    txtSprite1.pushToSprite(&background, text1XPos, yPos1, bgColor);
    //txtSprite2.setTextColor(TFT_RED, bgColor);
    //txtSprite2.fillSprite(bgColor);
    //txtSprite2.drawString(message2, 0, 0, 2);
    //txtSprite2.pushToSprite(&background, 18, 36, bgColor);
    
    if (overSprite == "kitty")
    {
        Kitty.pushImage(0, 0, kittyXSize, kittyYSize, kitty);
        Kitty.pushToSprite(&background, kittyXPos, kittyYPos, TFT_BLACK);
    }
    else if (overSprite == "kittyhappy")
    {
        Kitty.pushImage(0, 0, kittyXSize, kittyYSize, kittyhappy);
        Kitty.pushToSprite(&background, kittyXPos, kittyYPos, TFT_BLACK);
    }
    else if (overSprite == "kittyangry")
    {
        Kitty.pushImage(0, 0, kittyXSize, kittyYSize, kittyangry);
        Kitty.pushToSprite(&background, kittyXPos, kittyYPos, TFT_BLACK);
    }
    else if (overSprite == "wifi")
    {
        Charge.pushImage(0, 0, chargeXSize, chargeYSize, wifisetup);
        Charge.pushToSprite(&background, chargeXPos, chargeYPos, TFT_BLACK);
    }
    else if (overSprite == "connect")
    {
        Charge.pushImage(0, 0, chargeXSize, chargeYSize, wifisymbol);
        Charge.pushToSprite(&background, chargeXPos, chargeYPos, TFT_BLACK);
    }
    else if (overSprite == "charge")
    {
        Charge.pushImage(0, 0, chargeXSize, chargeYSize, charge);
        Charge.pushToSprite(&background, chargeXPos, chargeYPos, TFT_BLACK);
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

    chooseSprite = "wifi";

    text1Color = TFT_GREEN; // Note GREEN is actually RED!
    text1String = "Setup Wifi";    
    text1Offset = 17;

    // Display message note GREEN is RED, wifi setup
    displayImage(background, text1String, text1Color, TFT_BLACK, text1Offset,  text1YPos, "", chooseSprite);

    bool res;

    res = wm.autoConnect("Erwin Box Opener", "password"); // password protected ap

    if (!res)
    {
        Serial.println("Failed to connect");

        // ESP.restart();
    }

    chooseSprite = "connect";

    text1Color = TFT_WHITE;
    text1String = "Connecting";    
    text1Offset = 15;

    // Display message, wifi symbol
    displayImage(background, text1String, text1Color, TFT_BLACK, text1Offset, text1YPos, "", chooseSprite);

    delay(1000);

    // Save the custom parameters to FS
    if (shouldSaveConfig)
    {
        // Copy the string value
        strncpy(apiKey, api_text.getValue(), sizeof(apiKey));
        saveConfigFile();
    }
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

// Setup
void setup() {
  Serial.begin(115200);

  remainingTime.begin("charge_timer", false);

  pinMode(WIFI_PIN, INPUT_PULLUP);
  attachInterrupt(WIFI_PIN, handlewifiButtonPress, RISING);

  pinMode(CHARGE_PIN, INPUT_PULLUP);
  attachInterrupt(CHARGE_PIN, handlechargeButtonPress, RISING);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

      // Create graphics elements
  background.createSprite(backgroundXSize, backgroundYSize);
  background.setSwapBytes(true);
  Kitty.createSprite(kittyXSize, kittyYSize);
  Charge.createSprite(chargeXSize, chargeYSize);
  txtSprite1.createSprite(text1XSize, text1YSize);
  txtSprite2.createSprite(text2XSize, text2YSize);

  wificonnect();

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  Serial.println(WiFi.localIP()); // print local IP

  //get time each time the board starts
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  epochTime = getTime();
  rtc.setTime(epochTime);

  //update and store expiry time, if it doesn't exist set it to the current time plus 3 days
  expireTime = remainingTime.getUInt("timeExpire", epochTime + chargeTimer);
  remainingTime.putUInt("timeExpire", expireTime);
  remainingTime.end();  

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

  String percent = String(dataP) + "%";
  
  for (int i = 0; i < numGuesses; i++) {
    jsonDoc.add(mnemonics[i]);
  }

  Serial.printf("🔑️ Generated %u guesses\n", numGuesses);
  Serial.printf("➡️ Submitting to oracle\n");



  // Check if charge is needed and setup kitty or charge image
  if (chargeCheck < 10) {
    chooseSprite = "kitty";
  }
  else
  {
    chooseSprite = "charge";
  }

  text1Color = TFT_WHITE;
  text1String = "Generating";    
  text1Offset = 16;

  // Display message, sprite is kitty or charge
  displayImage(background, text1String, text1Color, TFT_BLACK, text1Offset, text1YPos, percent, chooseSprite);

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
      chooseSprite = "kittyhappy";

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
      chooseSprite = "kittyangry";

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
      chooseSprite = "kittyangry";

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
    chooseSprite = "kittyangry";

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
    chooseSprite = "charge";
  }

   // Do some maths to work out the successful percent of guess rounds
  dataP = float(dataS) / float(dataS + dataF + dataT) * 100;
  percent = String(dataP) + "%";
  
  // Display message, sprite is kitty or charge
  displayImage(background, text1String, text1Color, TFT_BLACK, text1Offset, text1YPos, percent, chooseSprite);

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
    if (chargebuttonPressed)
    {
        //update internal clock
        epochTime = getTime();
        rtc.setTime(epochTime);

        //update and store expiry time
        remainingTime.begin("charge_timer", false);
        expireTime = epochTime + chargeTimer;
        remainingTime.putUInt("timeExpire", expireTime);
        remainingTime.end(); 

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
