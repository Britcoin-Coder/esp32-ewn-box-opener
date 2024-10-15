/*
 * ------------------------------------------------------------------------
 * Project Name: esp32-ewn-box-opener
 * Description: Esp32 port of the Box Opener client for mining EWN tokens.
 * Author: Crey
 * Repository: https://github.com/cr3you/esp32-ewn-box-opener/
 * Fork Author: bigdaveakers
 * Fork Repository: https://github.com/Britcoin-Coder/esp32-ewn-box-opener
 * Date: 2024.10.13 
 * Version: 1.0.1
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

TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite background= TFT_eSprite(&tft);
TFT_eSprite txtSprite1= TFT_eSprite(&tft);
TFT_eSprite txtSprite2= TFT_eSprite(&tft);
TFT_eSprite Kitty= TFT_eSprite(&tft);
TFT_eSprite Charge= TFT_eSprite(&tft);

#define TRIGGER_PIN 0

//=====Box Opener client setup
const char *apiUrl = "https://api.erwin.lol/"; // mainnet
//const char *apiUrl = "https://devnet-api.erwin.lol/"; // devnet

// JSON configuration file
#define JSON_CONFIG_FILE "/apikey_config.json"

// Flag for saving API data
bool shouldSaveConfig = false;
 
// Variables to hold data from custom textboxes
char apiKey[300];

WiFiManager wm;

const int numGuesses = 50;
String mnemonics[numGuesses]; // bip39 mnemonic table
int sleepTime = 10000; // default sleep time in ms
int dataS = 0; //succesful data count
int dataF = 0; //failed data count
int dataT = 0; //Timeout count
int dataP = 0; //success percentage
int chargeCheck = 0; //count of consecutive rejects

void saveConfigFile()
// Save Config in JSON format
{
  Serial.println(F("Saving configuration..."));
  
  // Create a JSON document
  StaticJsonDocument<512> json;
  json["testString"] = apiKey;
 
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
 
bool loadConfigFile()
// Load existing configuration file
{
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
 
          strcpy(apiKey, json["testString"]);
 
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

 
void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
 
void configModeCallback(WiFiManager *myWiFiManager)
// Called when config mode launched
{
  Serial.println("Entered Configuration Mode");
 
  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
 
  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

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

  Serial.print("read api key: ");
  Serial.println(apiKey);


  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  // Create graphics elements
  background.createSprite(135,240);
  background.setSwapBytes(true);
  Kitty.createSprite(96,133);
  Charge.createSprite(77,88); 
  txtSprite1.createSprite(100,20);
  txtSprite2.createSprite(100,20);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.mode(WIFI_STA);
  
  //wm.resetSettings();

  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);
 
  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  // Define a text box, 50 characters maximum
  WiFiManagerParameter api_text("API", "Enter your API Key here", "default string", 256);
 
  // Add custom parameter
  wm.addParameter(&api_text);

  Serial.print("Connecting to WiFi ..");
  
  //Display empty box and Setup WiFi message
  background.pushImage(0,0,135,240,box);
  txtSprite1.setTextColor(TFT_GREEN,TFT_BLACK);
  txtSprite1.fillSprite(TFT_BLACK);
  txtSprite1.drawString("Setup Wifi",17,0,2); 
  txtSprite1.pushToSprite(&background,20,205,TFT_BLACK);
  //Render to screen
  background.pushSprite(0,0);

bool res;
 
res = wm.autoConnect("Erwin Box Opener","password"); // password protected ap

    if(!res) {
 
        Serial.println("Failed to connect");
 
        // ESP.restart();
 
    }

//Display empty box and Connecting message
  background.pushImage(0,0,135,240,box);
  txtSprite1.setTextColor(TFT_WHITE,TFT_BLACK);
  txtSprite1.fillSprite(TFT_BLACK);
  txtSprite1.drawString("Connecting",20,0,2); 
  txtSprite1.pushToSprite(&background,20,205,TFT_BLACK);
  //Render to screen
  background.pushSprite(0,0);

  delay(1000);
  
  Serial.print("api key: ");
  Serial.println(apiKey);

  // Save the custom parameters to FS
  if (shouldSaveConfig)
  {
    saveConfigFile();
     // Copy the string value
    strncpy(apiKey, api_text.getValue(), sizeof(apiKey));
  }

  Serial.print("api key: ");
  Serial.println(apiKey);
}



void setup()
{
  Serial.begin(115200);

  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  wificonnect();

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  Serial.println(WiFi.localIP()); // print local IP

  Serial.printf("===============\n");
  Serial.printf("Box-opener started\n");
}

//===== generate table of 50 bip39 12-word mnemonics
void generateMnemonics(String *mnemonics)
{
  for (int i = 0; i < numGuesses; i++)
  {
    std::vector<uint8_t> entropy(16); // 128-bit entropy -> 12 words
    for (size_t j = 0; j < entropy.size(); j++)
    {
      entropy[j] = esp_random() & 0xFF; // extract 1 random byte
    }
    BIP39::word_list mnemonic = BIP39::create_mnemonic(entropy);
    std::string mnemonicStr = mnemonic.to_string();
    mnemonics[i] = String(mnemonicStr.c_str());
    // Serial.printf("Generated mnemonic: %s\n", mnemonics[i].c_str());
  }
}

//===== submit mnemonics to Oracle
bool submitGuesses(String *mnemonics, const String &apiUrl, const String &apiKey)
{
  bool ret = false;
  DynamicJsonDocument jsonDoc(8192); // max size of the json output, to verify! (4kB was not enough)

  for (int i = 0; i < numGuesses; i++)
  {
    jsonDoc.add(mnemonics[i]);
  }

  Serial.printf("🔑️ Generated %u guesses\n", numGuesses);
  Serial.printf("➡️ Submitting to oracle\n");

  //Setup empty box and Generating message
  background.pushImage(0,0,135,240,box);
  txtSprite1.setTextColor(TFT_WHITE,TFT_BLACK);
  txtSprite1.fillSprite(TFT_BLACK);
  txtSprite1.drawString("Generating",17,0,2); 
  txtSprite1.pushToSprite(&background,20,205,TFT_BLACK);
  //Setup percent success text
  txtSprite2.pushToSprite(&background,18,36,TFT_BLACK);
  //Check if charge is needed and setup kitty or charge image
  if (chargeCheck < 10)
    {
      Kitty.pushImage(0,0,96,133,kitty);
      Kitty.pushToSprite(&background,26,52,TFT_BLACK);
    }
  //Render to Screen
  background.pushSprite(0,0);
 
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  HTTPClient http;
  http.begin(apiUrl + "/submit_guesses");
  http.addHeader("x-api-key", apiKey);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000); // increase default 5s to 15s

  int httpResponseCode = http.POST(jsonString);
  background.pushImage(0,0,135,240,box);
      
  if (httpResponseCode > 0)
  {  
    String response = http.getString();
    if (httpResponseCode == 202)
    {
      Serial.println("✅ Guesses accepted");
      //Setup Accepted message
      txtSprite1.setTextColor(TFT_BLUE,TFT_BLACK); //Note BLUE is actually GREEN!
      txtSprite1.fillSprite(TFT_BLACK);
      txtSprite1.drawString("Accepted",23,0,2);
      //Setup counts
      dataS ++;
      chargeCheck = 0;

      ret = false;
    }
    else if (httpResponseCode == 404) // "Closed Box Not Found"
    {
      Serial.printf("❌ Guesses rejected (%d): %s\n", httpResponseCode, response.c_str());
      //Setup No Boxes message
      txtSprite1.setTextColor(TFT_GREEN,TFT_BLACK); //Note GREEN is actually RED!
      txtSprite1.fillSprite(TFT_BLACK);
      txtSprite1.drawString("No Boxes",25,0,2);
      //Setup counts
      dataF ++;
      chargeCheck = 0; //if there is a valid response then the box doesn't need charging
      
      ret = false;
    }
    else // other errors
    {
      Serial.printf("❌ Guesses rejected (%d): %s\n", httpResponseCode, response.c_str());
      //Setup Rejected message
      txtSprite1.setTextColor(TFT_GREEN,TFT_BLACK); //Note GREEN is actually RED
      txtSprite1.fillSprite(TFT_BLACK);
      txtSprite1.drawString("Rejected",24,0,2);
      //Setup counts
      dataF ++;
      chargeCheck ++; //if a guess is rejected it COULD be due to needing to charge
      
      ret = true;
    }
  }
  else // even more other errors :V maybe do a reconnect?
  {
    Serial.printf("❌ Error in HTTP request: %s\n", http.errorToString(httpResponseCode).c_str());
    //Setup Timeout message
    txtSprite1.setTextColor(TFT_GREEN,TFT_BLACK); //Note Green is actually RED
    txtSprite1.fillSprite(TFT_BLACK);
    txtSprite1.drawString("Timeout",27,0,2);
    //Setup counts
    dataT ++;

    ret = true;
  }

  http.end();
    //If there are 10 consecutive rejects without and accept then it is likely that the box needs charging
    if (chargeCheck >= 10)
    {
      //Setup CHARGE ME message and charge image
      txtSprite1.setTextColor(TFT_GREEN,TFT_BLACK); //Note GREEN is actually RED
      txtSprite1.fillSprite(TFT_BLACK);
      txtSprite1.drawString("CHARGE ME",18,0,2);
      Charge.pushImage(0,0,77,88,charge);
      Charge.pushToSprite(&background,30,68,TFT_BLACK);
    }
    else
    { //if there is no charge needed setup the kitty image
      Kitty.pushImage(0,0,96,133,kitty);
      Kitty.pushToSprite(&background,26,52,TFT_BLACK);
    }
  
    //do some maths to work out the succesful percent of guess rounds
    dataP = float(dataS)/float(dataS+dataF+dataT)*100;
    String percent = String(dataP)+"%";
    //Setup status message and percent message
    txtSprite1.pushToSprite(&background,20,205,TFT_BLACK);
    txtSprite2.setTextColor(TFT_RED,TFT_BLACK);
    txtSprite2.fillSprite(TFT_BLACK);
    txtSprite2.drawString(percent,0,0,2);
    txtSprite2.pushToSprite(&background,13,37,TFT_BLACK);
    //Render to screem
    background.pushSprite(0,0);
    
    Serial.println(apiKey);
    
    
  return ret;
}

//====== main loop ====
void loop()
{
  // is configuration portal requested?
  if ( digitalRead(TRIGGER_PIN) == LOW) 
    {
      wm.resetSettings();
      wificonnect();
    }


  //--- reconnect wifi if it is not connected by some reason
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("WiFi disconnected, trying to reconnect..\n");
    WiFi.disconnect();
    WiFi.reconnect();
  }
    
  Serial.println("⚙️ Generating guesses...");

  generateMnemonics(mnemonics);

  bool rateLimited = submitGuesses(mnemonics, apiUrl, (const char*)apiKey);

  if (rateLimited)
  {
    sleepTime += 10000;
  }
  else
  {
    sleepTime -= 1000;
  }

  if (sleepTime < 10000)
  {
    sleepTime = 10000;
  }
  if (sleepTime > 60000) // if sleep for more than a minute limit it to one minute
  {
    sleepTime = 60000;
  }

  Serial.printf("waiting %is for next batch...\n", sleepTime/1000);
  delay(sleepTime);
}
