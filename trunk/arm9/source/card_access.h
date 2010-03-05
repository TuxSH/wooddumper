/*
    card_access.h
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

#ifndef __CARD_ACCESS_H__
#define __CARD_ACCESS_H__

#include <nds.h>

class CCard
{
  public:
    CCard();
    ~CCard();
    void Init(void);
    const u8* SecureArea(void) {return iSecureArea;};
    u32 SecureAreaSize(void) {return EInitAreaSize;};
    u32 CartSize(void) {return (128*1024)<<iHeader->deviceSize;};
    const char* Name(void) {return iName;};
    const u8* Buffer(u32 anAddress);
    u32 BufferSize(void) {return EBufferSize;};
  private:
    u8* iSecureArea;
    u8* iBuffer;
    const tNDSHeader* iHeader;
    u32 iCardId;
    u32* iCardHash;
    u32 iKeyCode[3];
    u32 iFlags;
    bool iCheapCard;
    u32 iGameCode;
    char iName[5];
  private:
    enum
    {
      EInitAreaSize=0x8000,
      ESecureAreaSize=0x4000,
      ESecureAreaOffset=0x4000,
      EBufferSize=0x8000,
      EBlockSize=0x200,
      EHashSize=0x412,
      EEncMudule=2
    };
  private:
    struct {
      unsigned int iii;
      unsigned int jjj;
      unsigned int kkkkk;
      unsigned int llll;
      unsigned int mmm;
      unsigned int nnn;
    } iKey1;
  private:
    void Panic(void);
    int Key(void);
    void Reset(void);
    void Header(u8* aHeader);
    void ReadHeader(void);
    void ReadSecureArea(void);
    void InitKey1(u8* aCmdData);
    void CreateEncryptedCommand(u8 aCommand,u8* aCmdData,u32 aBlock);
    void Delay(u16 aTimeout);
    void DecryptSecureArea(void);
  private: //encryption
    void InitKey(u32 aGameCode,bool aType);
    void ApplyKey(void);
    void CryptUp(u32* aPtr);
    void CryptDown(u32* aPtr);
};

inline u32 gamecode(const char* aGameCode) {u32 gameCode; memcpy(&gameCode,aGameCode,sizeof(gameCode)); return gameCode;}

// chosen by fair dice roll.
// guaranteed to be random.
#define getRandomNumber() (4)

#endif
