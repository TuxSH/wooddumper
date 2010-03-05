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

class CSocket2
{
  public:
    CSocket2(bool aBlock=true);
    void Bind(unsigned short aPort);
    void Connect(const char* anAddress,unsigned short aPort);
    void Listen(void);
    CSocket2* Accept(void);
    int Receive(char* aBuffer,int aSize);
    int Send(const char* aBuffer,int aSize);
    int Send(const char* aBuffer);
    void SetBlock(bool aBlock=true);
    ~CSocket2();
  private:
    void Panic(void);
  private:
    int iSocket;
    bool iBlock;
    CSocket2(int aSocket,bool aBlock);
};

#endif
