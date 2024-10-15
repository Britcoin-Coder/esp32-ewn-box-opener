# esp32-ewn-box-opener
Esp32 port of the Box Opener client for mining EWN tokens.

Original Box Opener client written in python: https://github.com/Erwin-Schrodinger-Token/ewn-box-opener

Forked from https://github.com/cr3you/esp32-ewn-box-opener 

For easy import I suggest using VSCode with PlatformIO plugin.

This projest uses ciband/bip39 library and esp32 hardware random number generator to create mnemonics.

**>>I tested this code on T-Display<<**

This is a work in progress. There are more features to be added (like GUI to set wifi credencials or the API key).
But for now it should just work.

## Importing project.
**The easiest way to import is to have VSCode (Visual Studio Code editor) installed with PlatformIO extension on your computer.**

Download whole repository to your disk (the green "Code" button somewhere in upper right-> Download ZIP).

Extract the **esp32-ewn-box-opener-main** direcotry to your disk.

Open VSCode, open PlatformIO extension (usually the ant head icon on the left strip).

Choose "Pick a folder" and open your extracted folder.

Let PlatformIO do its thing, it can take a while if you import the project for the first time.

Alternatively you could just right click on your folder and choose "open with Code"...

## Setting up.
Get your API key from:

Mainnet - https://erwin.lol/box-opener

Devnet - https://devnet.erwin.lol/box-opener

If you have different board than T-Display then additional changes will be required

You will need to add the TFT_eSPI library to your project, do this from the platformio Quick Access>Library menu and in
the Registry tab search for TFT_eSPI. Select is and then click the Add to Project button and select the right project.

Once the Library is added you need to update the User_SetupSelect.h file to setup for the t-display.
comment this line out

#include <User_Setup.h>           // Default setup is root library folder

```
//#include <User_Setup.h>           // Default setup is root library folder
``````
and uncomment this line

//#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT

```
#include <User_Setups/Setup25_TTGO_T_Display.h>    // Setup file for ESP32 and TTGO T-Display ST7789V SPI bus TFT
``````
You will also need to add the WiFi Manager library to your project, do this from the platformio Quick Access>Library menu and in
the Registry tab search for WiFi Manager by tzapu
Credit to https://dronebotworkshop.com/wifimanager/ for the instructions!

Compile and upload to your board (click the right arrow button on bottom strip of the VSCode).

Once running you will need to setup your credentials. To connect to the board use a device to connect to the WiFi Access Point that should show in your WiFi network list as "Erwin Box Opener" the password is "password"
From here choose the configure WiFi button and you should be presented with a screen where you can input your SSID, password and API Key.
## NOTE, to reset your creds press and hold the left button until the screen shows an empty box and Setup WiFi message then connect to the network "Erwin Box Opener" again. Reset your creds and off you go!
```
//wm.resetSettings();
``````
Once running you should see the below on the serial terminal and the lcd screen should do its thing!

You can unplug the board from your computer and the settings should persist.

## Running.
Open serial terminal with 115200 bps baudrate and connect to the esp32 board.

If everything is OK you should see something like this:
```
Connecting to WiFi ...
192.168.145.183
===============
Box-opener started
⚙️ Generating guesses...
🔑️ Generated 50 guesses
➡️ Submitting to oracle
✅ Guesses accepted
waiting 10s for next batch...
⚙️ Generating guesses...
🔑️ Generated 50 guesses
➡️ Submitting to oracle
✅ Guesses accepted
waiting 10s for next batch...
⚙️ Generating guesses...
🔑️ Generated 50 guesses
➡️ Submitting to oracle
✅ Guesses accepted
waiting 10s for next batch...
```
You should also see the kitty in a box on your t-display screen and various status messages to show that it is running.
