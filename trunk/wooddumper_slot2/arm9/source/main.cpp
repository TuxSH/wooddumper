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
#include <time.h>
#include <stdio.h>
#include <sys/dirent.h>
#include <errno.h>
#include <sys/iosupport.h>

#include <fcntl.h>
#include <unistd.h>

#include <fat.h>

#include "card_access.h"
#include "../../../version.h"

static bool DumpInternal(FILE* aF,CCard* aCard)
{
  bool result=true;
  u32 cardSize=aCard->CartSize(),pos=0,block=aCard->BufferSize();
  u32 written=fwrite(aCard->SecureArea(),1,aCard->SecureAreaSize(),aF);
  if(written!=aCard->SecureAreaSize())
  {
    iprintf("write(1) error\n");
    result=false;
  }
  if(result)
  {
    pos+=aCard->SecureAreaSize();
    while(pos<cardSize)
    {
      u32 written=fwrite(aCard->Buffer(pos),1,block,aF);
      if(written!=block)
      {
        iprintf("write(2) error\n");
        result=false;
        break;
      }
      pos+=block;
      iprintf("\x1b[32Ddumped %u bytes",pos);
    }
    iprintf("\n");
  }
  return result;
}

static bool Dump(void)
{
  bool result=false;
  CCard* card=new CCard;
  if(card->Init())
  {
    char filename[128];
    time_t epochTime;
    tm timeParts;

    if(time(&epochTime)==(time_t)-1)
    {
      memset(&timeParts,0,sizeof(timeParts));
    }
    else
    {
      localtime_r(&epochTime,&timeParts);
    }

    sprintf(filename,"fat:/%04d-%02d-%02d-%02d-%02d-%02d-%s.nds",timeParts.tm_year+1900,timeParts.tm_mon+1,timeParts.tm_mday,timeParts.tm_hour,timeParts.tm_min,timeParts.tm_sec,card->Name());

    FILE* f=fopen(filename,"wb");
    if(f)
    {
      result=DumpInternal(f,card);
      fclose(f);
    }
    else
    {
      iprintf("open file error\n");
    }
  }
  delete card;
  return result;
}

int main(void)
{
  sysSetBusOwners(BUS_OWNER_ARM9,BUS_OWNER_ARM9);
  PrintConsole topScreen;
  PrintConsole bottomScreen;
  videoSetMode(MODE_0_2D);
  videoSetModeSub(MODE_0_2D);
  vramSetBankA(VRAM_A_MAIN_BG);
  vramSetBankC(VRAM_C_SUB_BG);
  consoleInit(&topScreen,3,BgType_Text4bpp,BgSize_T_256x256,31,0,true,true);
  consoleInit(&bottomScreen,3,BgType_Text4bpp,BgSize_T_256x256,31,0,false, true);

  consoleSelect(&topScreen);
  iprintf("version: %s\n",WOOD_VERSION);
  consoleSelect(&bottomScreen);

  if(fatInitDefault())
  {
    while(true)
    {
      if(Dump()) iprintf("\x1b[32;1mdump success.\x1b[39;0m");
      else iprintf("\x1b[31;1mdump fail.\x1b[39;0m");
      iprintf(" EXTRACT card.\n\n");
    }
  }
  else iprintf("fat init fail\n");

  while(true) swiWaitForVBlank();

  return 0;
}
