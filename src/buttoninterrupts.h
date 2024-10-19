#include "ewnconfig.h"

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