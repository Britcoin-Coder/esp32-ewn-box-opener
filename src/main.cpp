/*
 * ------------------------------------------------------------------------
 * Project Name: esp32-ewn-box-opener
 * Description: Esp32 port of the Box Opener client for mining EWN tokens.
 * Author: Crey
 * Repository: https://github.com/cr3you/esp32-ewn-box-opener/
 * Date: 2024.10.04 
 * Version: 1.0.1
 * License: MIT
 * ------------------------------------------------------------------------
 */

#include <string>
#include "bip39/bip39.h"
#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h> 
#include "kitty.h"
TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite background= TFT_eSprite(&tft);
TFT_eSprite txtSprite1= TFT_eSprite(&tft);
TFT_eSprite txtSprite2= TFT_eSprite(&tft);


//=====wifi setup
const char *ssid = ""; // <---------------------- SET THIS !!!
const char *password = ""; // <-------------- SET THIS !!!

//=====Box Opener client setup
const char *apiUrl = "https://api.erwin.lol/"; // mainnet
//const char *apiUrl = "https://devnet-api.erwin.lol/"; // devnet
const char *apiKey = ""; // <---------------------- SET THIS !!!


const int numGuesses = 50;
String mnemonics[numGuesses]; // bip39 mnemonic table
int sleepTime = 10000; // default sleep time in ms
int dataS = 0; //succesful data count
int dataF = 0; //failed data count
int dataT = 0; //Timeout count
int dataP = 0; //success percentage


void setup()
{
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  background.createSprite(135,240);
  background.setSwapBytes(true);
  txtSprite1.createSprite(100,20);
  txtSprite2.createSprite(100,20);
  
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  
  background.pushImage(0,0,135,240,kitty);
  txtSprite1.setTextColor(TFT_WHITE,TFT_BLACK);
  txtSprite1.fillSprite(TFT_BLACK);
  txtSprite1.drawString("Connecting",20,0,2);
  txtSprite1.pushToSprite(&background,20,205,TFT_BLACK);
  background.pushSprite(0,0);

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
  background.pushImage(0,0,135,240,kitty);
  txtSprite1.setTextColor(TFT_WHITE,TFT_BLACK);
  txtSprite1.fillSprite(TFT_BLACK);
  txtSprite1.drawString("Generating",17,0,2);
  txtSprite1.pushToSprite(&background,20,205,TFT_BLACK);
  txtSprite2.pushToSprite(&background,18,36,TFT_BLACK);
  background.pushSprite(0,0);
 
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  HTTPClient http;
  http.begin(apiUrl + "/submit_guesses");
  http.addHeader("x-api-key", apiKey);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000); // increase default 5s to 15s

  int httpResponseCode = http.POST(jsonString);
  background.pushImage(0,0,135,240,kitty);
      
  if (httpResponseCode > 0)
  {  
    String response = http.getString();
    if (httpResponseCode == 202)
    {
      Serial.println("✅ Guesses accepted");
      txtSprite1.setTextColor(TFT_BLUE,TFT_BLACK);
      txtSprite1.fillSprite(TFT_BLACK);
      txtSprite1.drawString("Accepted",23,0,2);
      dataS ++;
      ret = false;
    }
    else if (httpResponseCode == 404) // "Closed Box Not Found"
    {
      Serial.printf("❌ Guesses rejected (%d): %s\n", httpResponseCode, response.c_str());
      txtSprite1.setTextColor(TFT_GREEN,TFT_BLACK);
      txtSprite1.fillSprite(TFT_BLACK);
      txtSprite1.drawString("No Boxes",25,0,2);
      dataF ++;
      ret = false;
    }
    else // other errors
    {
      Serial.printf("❌ Guesses rejected (%d): %s\n", httpResponseCode, response.c_str());
      txtSprite1.setTextColor(TFT_GREEN,TFT_BLACK);
      txtSprite1.fillSprite(TFT_BLACK);
      txtSprite1.drawString("Rejected",24,0,2);
      dataF ++;
      ret = true;
    }
  }
  else // even more other errors :V maybe do a reconnect?
  {
    Serial.printf("❌ Error in HTTP request: %s\n", http.errorToString(httpResponseCode).c_str());
    txtSprite1.setTextColor(TFT_GREEN,TFT_BLACK);
    txtSprite1.fillSprite(TFT_BLACK);
    txtSprite1.drawString("Timeout",27,0,2);
    
    dataT ++;
    ret = true;
  }

  http.end();
    dataP = float(dataS)/float(dataS+dataF+dataT)*100;
    String percent = String(dataP)+"%";
    txtSprite1.pushToSprite(&background,20,205,TFT_BLACK);
    txtSprite2.setTextColor(TFT_RED,TFT_BLACK);
    txtSprite2.fillSprite(TFT_BLACK);
    txtSprite2.drawString(percent,0,0,2);
    txtSprite2.pushToSprite(&background,13,37,TFT_BLACK);
    background.pushSprite(0,0);
  return ret;
}

//====== main loop ====
void loop()
{
  //--- reconnect wifi if it is not connected by some reason
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("WiFi disconnected, trying to reconnect..\n");
    WiFi.disconnect();
    WiFi.reconnect();
  }
    
  Serial.println("⚙️ Generating guesses...");

  generateMnemonics(mnemonics);

  bool rateLimited = submitGuesses(mnemonics, apiUrl, apiKey);

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
