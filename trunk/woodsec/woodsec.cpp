/*
    woodsec.cpp
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

#include <stdio.h>
#include <string.h>

bool EnDecryptSecureArea(bool bEncrypt);

FILE* fNDS=NULL;

int main(int argc,char *argv[])
{
  printf("woodsec v0.90 - tool to manipulate secure area.\nproduced by yellow wood goblin.\n\n");
  if(argc<3)
  {
    printf("USAGE:\n\twoodsec e filename\t\tencrypt secure area.\n\twoodsec d filename\t\tdecrypt secure area.\n");
    return 1;
  }
  int result=0;
  fNDS=fopen(argv[2],"r+b");
  if(fNDS)
  {
    if(stricmp("e",argv[1])==0) //encrypt
    {
      printf("encrypting %s.\n",argv[2]);
      if(!EnDecryptSecureArea(true)) result=1;
    }
    else if(stricmp("d",argv[1])==0) //decrypt
    {
      printf("decrypting %s.\n",argv[2]);
      if(!EnDecryptSecureArea(false)) result=1;
    }
    else
    {
      printf("invalid option.\n");
      result=1;
    }
    fclose(fNDS);
  }
  else
  {
    printf("cant' open file %s.\n",argv[2]);
    result=1;
  }
  return result;
}
