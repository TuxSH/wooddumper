/*
    main.cpp
    Copyright (C) 2010 yellow wood goblin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nds.h>
#include <dswifi7.h>
#include <maxmod7.h>

void VcountHandler(void)
{
  inputGetAndSend();
}

void VblankHandler(void)
{
  Wifi_Update();
}


int main(void)
{
  irqInit();
  fifoInit();

  // read User Settings from firmware
  readUserSettings();

  // Start the RTC tracking IRQ
  initClockIRQ();

  SetYtrigger(80);

  installWifiFIFO();
  installSoundFIFO();

  installSystemFIFO();

  irqSet(IRQ_VCOUNT, VcountHandler);
  irqSet(IRQ_VBLANK, VblankHandler);

  irqEnable(IRQ_VBLANK|IRQ_VCOUNT|IRQ_NETWORK);

  while(true) swiWaitForVBlank();
}
