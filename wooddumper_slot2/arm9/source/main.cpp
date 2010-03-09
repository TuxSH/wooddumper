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
#include <stdio.h>
#include <sys/dirent.h>
#include <errno.h>
#include <sys/iosupport.h>

#include <fcntl.h>
#include <unistd.h>

#include <fat.h>

#include "card_access.h"

static bool Dump(void)
{
  CCard* card=new CCard;
  card->Init();
  char filename[128];
  sprintf(filename,"fat:/%s.nds",card->Name());

  FILE* f=fopen(filename,"wb");
  if(f)
  {
    u32 cardSize=card->CartSize(),pos=0,block=card->BufferSize();
    u32 written=fwrite(card->SecureArea(),1,card->SecureAreaSize(),f);
    if(written!=card->SecureAreaSize())
    {
      iprintf("write(1) error\n");
      fclose(f);
      return false;
    }
    pos+=card->SecureAreaSize();
    while(pos<cardSize)
    {
      u32 written=fwrite(card->Buffer(pos),1,block,f);
      if(written!=block)
      {
        iprintf("write(2) error\n");
        fclose(f);
        return false;
      }
      pos+=block;
      iprintf("\x1b[32Ddumped %u bytes",pos);
    }
    iprintf("\n");
    fclose(f);
  }
  else
  {
    iprintf("open file error\n");
    return false;
  }
  return true;
}

int main(void)
{
  sysSetBusOwners(BUS_OWNER_ARM9,BUS_OWNER_ARM9);
  consoleDemoInit();

  if(fatInitDefault())
  {
    if(Dump()) iprintf("dump success\n");
    else iprintf("dump fail\n");
  }
  else iprintf("fat init fail\n");

  while(true) swiWaitForVBlank();

  return 0;
}
