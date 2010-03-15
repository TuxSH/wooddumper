/*
    card_access.cpp
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
#include "card_access.h"
#include "key1.h"

void CCard::Panic(void)
{
  while(true) swiWaitForVBlank();
}

CCard::CCard(): iSecureArea(NULL),iBuffer(NULL),iCardId(0),iCardHash(NULL),iFlags(0),iCheapCard(false),iGameCode(0),iOk(true)
{
  iSecureArea=new u8[EInitAreaSize];
  memset(iSecureArea,0,EInitAreaSize);
  iBuffer=new u8[EBufferSize];
  iHeader=(const tNDSHeader*)iSecureArea;
  iCardHash=new u32[EHashSize];
}

CCard::~CCard()
{
  delete[] iCardHash;
  delete[] iBuffer;
  delete[] iSecureArea;
}

int CCard::Key(void)
{
  scanKeys();
  return keysUp();
}

bool CCard::Init(void)
{
  iprintf("Set a Target card.\n<A>:OK\n");
  while(!(Key()&KEY_A));
  iOk=true;
  memset(iSecureArea,0,EInitAreaSize);
  ReadHeader();
  ReadSecureArea();
  memcpy(iName,iHeader->gameCode,4);
  iName[4]=0;
  return iOk;
}

void CCard::Reset(void)
{
  u8 cmdData[8]={0,0,0,0,0,0,0,CARD_CMD_DUMMY};
  cardWriteCommand(cmdData);
  CARD_CR2=CARD_ACTIVATE|CARD_nRESET|CARD_CLK_SLOW|CARD_BLK_SIZE(5)|CARD_DELAY2(0x18);
  u32 readed=0;
  do
  {
    if(CARD_CR2&CARD_DATA_READY)
    {
      if(readed<0x2000)
      {
        u32 data=CARD_DATA_RD;
        (void)data;
        readed+=4;
      }
    }
  } while(CARD_CR2&CARD_BUSY);
}

void CCard::Header(u8* aHeader)
{
  if(iCheapCard)
  {
    //this is magic of wood goblins
    for(size_t ii=0;ii<8;++ii)
      cardParamCommand(CARD_CMD_HEADER_READ,ii*0x200,CARD_ACTIVATE|CARD_nRESET|CARD_CLK_SLOW|CARD_BLK_SIZE(1)|CARD_DELAY1(0x1FFF)|CARD_DELAY2(0x3F),(u32*)(aHeader+ii*0x200),0x200);
  }
  else
  {
    cardParamCommand(CARD_CMD_HEADER_READ,0,CARD_ACTIVATE|CARD_nRESET|CARD_CLK_SLOW|CARD_BLK_SIZE(4)|CARD_DELAY1(0x1FFF)|CARD_DELAY2(0x3F),(u32*)aHeader,0x1000);
  }
}

void CCard::ReadHeader(void)
{
  CARD_CR2=0;
  CARD_CR1H=0;
  swiDelay(167550);
  CARD_CR1H=CARD_CR1_ENABLE|CARD_CR1_IRQ;
  CARD_CR2=CARD_nRESET|CARD_SEC_SEED;
  while(CARD_CR2&CARD_BUSY) ;
  Reset();
  while(CARD_CR2&CARD_BUSY) ;
  iCardId=cardReadID(0);
  iprintf("card id: 0x%x\n",iCardId);
  while(CARD_CR2&CARD_BUSY) ;
  iCheapCard=iCardId&0x80000000;
  if(iOk) Header(iSecureArea);
}

void CCard::GetIDSafe(uint32 flags,const uint8* command)
{
  u32 data;
  cardWriteCommand(command);
  CARD_CR2=flags|CARD_BLK_SIZE(7);
  do
  {
    if(CARD_CR2&CARD_DATA_READY)
    {
      data=CARD_DATA_RD;
      if(data!=iCardId)
      {
        printf("secure cardid fail:%x\n",data);
        iOk=false;
        break;
      }
    }
  } while(CARD_CR2&CARD_BUSY);
}

void CCard::ReadSecureArea(void)
{
  iGameCode=gamecode(iHeader->gameCode);
  InitKey(iGameCode,false);
  u32 flagsKey1=CARD_ACTIVATE|CARD_nRESET|(iHeader->cardControl13&(CARD_WR|CARD_CLK_SLOW))|((iHeader->cardControlBF&(CARD_CLK_SLOW|CARD_DELAY1(0x1FFF)))+((iHeader->cardControlBF&CARD_DELAY2(0x3F))>>16));
  u32 flagsSec=(iHeader->cardControlBF&(CARD_CLK_SLOW|CARD_DELAY1(0x1FFF)|CARD_DELAY2(0x3F)))|CARD_ACTIVATE|CARD_nRESET|CARD_SEC_EN|CARD_SEC_DAT;
  ALIGN(4) u8 cmdData[8];
  const u8 cardSeedBytes[]={0xE8,0x4D,0x5A,0xB1,0x17,0x8F,0x99,0xD5};
  u32* secureArea=(u32*)(iSecureArea+ESecureAreaOffset);
  iFlags=iHeader->cardControl13&~CARD_BLK_SIZE(7);
  if(!iCheapCard) flagsKey1|=CARD_SEC_LARGE;
  InitKey1(cmdData);
  cardPolledTransfer((iHeader->cardControl13&(CARD_WR|CARD_nRESET|CARD_CLK_SLOW))|CARD_ACTIVATE,NULL,0,cmdData);
  CreateEncryptedCommand(CARD_CMD_ACTIVATE_SEC,cmdData,0);
  if(iCheapCard)
  {
    cardPolledTransfer(flagsKey1,NULL,0,cmdData);
    Delay(iHeader->readTimeout);
  }
  cardPolledTransfer(flagsKey1,NULL,0,cmdData);

  CARD_CR2=0;
  CARD_1B0=cardSeedBytes[iHeader->deviceType&0x07]|(iKey1.nnn<<15)|(iKey1.mmm<<27)|0x6000;
  CARD_1B4=0x879b9b05;
  CARD_1B8=iKey1.mmm>>5;
  CARD_1BA=0x5c;
  CARD_CR2=CARD_nRESET|CARD_SEC_SEED|CARD_SEC_EN|CARD_SEC_DAT;
  flagsKey1|=CARD_SEC_EN|CARD_SEC_DAT;

  CreateEncryptedCommand(CARD_CMD_SECURE_CHIPID,cmdData,0);

  if(iCheapCard)
  {
    cardPolledTransfer(flagsKey1,NULL,0,cmdData);
    Delay(iHeader->readTimeout);
  }
  GetIDSafe(flagsKey1,cmdData);
  if(!iOk) return;

  for(int secureBlockNumber=4;secureBlockNumber<8;++secureBlockNumber)
  {
    CreateEncryptedCommand(CARD_CMD_SECURE_READ,cmdData,secureBlockNumber);
    if (iCheapCard)
    {
      cardPolledTransfer(flagsSec,NULL,0,cmdData);
      Delay(iHeader->readTimeout);
      for(int ii=8;ii>0;--ii)
      {
        cardPolledTransfer(flagsSec|CARD_BLK_SIZE(1),secureArea,0x200,cmdData);
        secureArea+=0x200/sizeof(u32);
      }
    }
    else
    {
      cardPolledTransfer(flagsSec|CARD_BLK_SIZE(4)|CARD_SEC_LARGE,secureArea,0x1000,cmdData);
      secureArea+=0x1000/sizeof(u32);
    }
  }
  CreateEncryptedCommand(CARD_CMD_DATA_MODE,cmdData,0);

  if(iCheapCard)
  {
    cardPolledTransfer(flagsKey1,NULL,0,cmdData);
    Delay(iHeader->readTimeout);
  }
  cardPolledTransfer(flagsKey1,NULL,0,cmdData);

  DecryptSecureArea();
  secureArea=(u32*)(iSecureArea+ESecureAreaOffset);
  if(secureArea[0]==0x72636e65/*'encr'*/&&secureArea[1]==0x6a624f79/*'yObj'*/)
  {
    secureArea[0]=0xe7ffdeff;
    secureArea[1]=0xe7ffdeff;
    iprintf("secure area ok.\n");
  }
  else
  {
    iprintf("\x1b[31;1msecure area error.\x1b[39;0m\n");
    //dragon quest 5 has invalid secure area. really.
    //iOk=false;
  }
}

void CCard::InitKey(u32 aGameCode,bool aType)
{
  memcpy(iCardHash,gEncrData,sizeof(gEncrData));
  iKeyCode[0]=aGameCode;
  iKeyCode[1]=aGameCode/2;
  iKeyCode[2]=aGameCode*2;

  ApplyKey();
  ApplyKey();
  iKeyCode[1]=iKeyCode[1]*2;
  iKeyCode[2]=iKeyCode[2]/2;
  if(aType) ApplyKey();
}

void CCard::ApplyKey(void)
{
  u32 scratch[2];

  CryptUp(&iKeyCode[1]);
  CryptUp(&iKeyCode[0]);
  memset(scratch,0,sizeof(scratch));

  for(int ii=0;ii<0x12;++ii)
  {
    iCardHash[ii]=iCardHash[ii]^bswap(iKeyCode[ii%2]);
  }
  for(int ii=0;ii<EHashSize;ii+=2)
  {
    CryptUp(scratch);
    iCardHash[ii]=scratch[1];
    iCardHash[ii+1]=scratch[0];
  }
}

void CCard::CryptUp(u32* aPtr)
{
  u32 x=aPtr[1];
  u32 y=aPtr[0];
  u32 z;
  for(int ii=0;ii<0x10;++ii)
  {
    z=iCardHash[ii]^x;
    x=iCardHash[0x012+((z>>24)&0xff)];
    x=iCardHash[0x112+((z>>16)&0xff)]+x;
    x=iCardHash[0x212+((z>>8)&0xff)]^x;
    x=iCardHash[0x312+((z>>0)&0xff)]+x;
    x=y^x;
    y=z;
  }
  aPtr[0]=x^iCardHash[0x10];
  aPtr[1]=y^iCardHash[0x11];
}

void CCard::CryptDown(u32* aPtr)
{
  u32 x=aPtr[1];
  u32 y=aPtr[0];
  u32 z;
  for(int ii=0x11;ii>0x01;--ii)
  {
    z=iCardHash[ii]^x;
    x=iCardHash[0x012+((z>>24)&0xff)];
    x=iCardHash[0x112+((z>>16)&0xff)]+x;
    x=iCardHash[0x212+((z>>8)&0xff)]^x;
    x=iCardHash[0x312+((z>>0)&0xff)]+x;
    x=y^x;
    y=z;
  }
  aPtr[0]=x^iCardHash[0x01];
  aPtr[1]=y^iCardHash[0x00];
}

void CCard::InitKey1(u8* aCmdData)
{
  iKey1.iii=getRandomNumber()&0x00000fff;
  iKey1.jjj=getRandomNumber()&0x00000fff;
  iKey1.kkkkk=getRandomNumber()&0x000fffff;
  iKey1.llll=getRandomNumber()&0x0000ffff;
  iKey1.mmm=getRandomNumber()&0x00000fff;
  iKey1.nnn=getRandomNumber()&0x00000fff;

  aCmdData[7]=CARD_CMD_ACTIVATE_BF;
  aCmdData[6]=(u8)(iKey1.iii>>4);
  aCmdData[5]=(u8)((iKey1.iii<<4)|(iKey1.jjj>>8));
  aCmdData[4]=(u8)iKey1.jjj;
  aCmdData[3]=(u8)(iKey1.kkkkk>>16);
  aCmdData[2]=(u8)(iKey1.kkkkk>>8);
  aCmdData[1]=(u8)iKey1.kkkkk;
  aCmdData[0]=(u8)getRandomNumber();
}

void CCard::CreateEncryptedCommand(u8 aCommand,u8* aCmdData,u32 aBlock)
{
  u32 iii,jjj;
  if(aCommand!=CARD_CMD_SECURE_READ) aBlock=iKey1.llll;
  if(aCommand==CARD_CMD_ACTIVATE_SEC)
  {
    iii=iKey1.mmm;
    jjj=iKey1.nnn;
  }
  else
  {
    iii=iKey1.iii;
    jjj=iKey1.jjj;
  }
  aCmdData[7]=(u8)(aCommand|(aBlock>>12));
  aCmdData[6]=(u8)(aBlock>>4);
  aCmdData[5]=(u8)((aBlock<<4)|(iii>>8));
  aCmdData[4]=(u8)iii;
  aCmdData[3]=(u8)(jjj>>4);
  aCmdData[2]=(u8)((jjj<<4)|(iKey1.kkkkk>>16));
  aCmdData[1]=(u8)(iKey1.kkkkk>>8);
  aCmdData[0]=(u8)iKey1.kkkkk;
  CryptUp((u32*)aCmdData);
  iKey1.kkkkk+=1;
}

void CCard::Delay(u16 aTimeout)
{
  TIMER_DATA(0)=0-(((aTimeout&0x3FFF)+3));
  TIMER_CR(0)=TIMER_DIV_256|TIMER_ENABLE;
  while(TIMER_DATA(0)!=0xFFFF);
  TIMER_CR(0)=0;
  TIMER_DATA(0)=0;
}

void CCard::DecryptSecureArea(void)
{
  u32* secureArea=(u32*)(iSecureArea+ESecureAreaOffset);
  InitKey(iGameCode,false);
  CryptDown(secureArea);
  InitKey(iGameCode,true);
  for(int ii=0;ii<0x200;ii+=2) CryptDown(secureArea+ii);
}

const u8* CCard::Buffer(u32 anAddress)
{
  if(anAddress&(EBufferSize-1))
  {
    iprintf("invalid address\n");
    Panic();
  }
  for(size_t ii=0;ii<(EBufferSize/EBlockSize);++ii)
  {
    cardParamCommand(CARD_CMD_DATA_READ,anAddress+ii*0x200,iFlags|CARD_ACTIVATE|CARD_nRESET|CARD_BLK_SIZE(1),(u32*)(iBuffer+ii*0x200),EBlockSize);
  }
  return iBuffer;
}
