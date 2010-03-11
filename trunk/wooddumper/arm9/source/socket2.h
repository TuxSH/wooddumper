/*
    socket2.h
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

#ifndef __SOCKET2_H__
#define __SOCKET2_H__

typedef bool (*TSendCallback)(void *aParam);

class CSocket2
{
  public:
    CSocket2(bool aNonBlock=true);
    void Bind(unsigned short aPort);
    void Connect(const char* anAddress,unsigned short aPort);
    void Listen(void);
    CSocket2* Accept(bool aWait);
    int Receive(char* aBuffer,int aSize,bool aWait);
    int Send(const char* aBuffer,int aSize,TSendCallback aCallBack,void *aParam);
    int Send(const char* aBuffer);
    void SetNonBlock(bool aNonBlock=true);
    ~CSocket2();
  private:
    void Panic(void);
  private:
    int iSocket;
    bool iNonBlock;
    CSocket2(int aSocket,bool aNonBlock);
};

#endif
