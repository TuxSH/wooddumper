/*
    socket2.cpp
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
#include <errno.h>
#include <netinet/in.h>
#include "socket2.h"

void CSocket2::Panic(void)
{
  while(true) swiWaitForVBlank();
}

CSocket2::CSocket2(bool aNonBlock): iSocket(-1)
{
  iSocket=socket(AF_INET,SOCK_STREAM,0);
  if(iSocket<0)
  {
    iprintf("socket error\n"); Panic();
  }
  SetNonBlock(aNonBlock);
}

CSocket2::CSocket2(int aSocket,bool aNonBlock): iSocket(aSocket)
{
  SetNonBlock(aNonBlock);
}

CSocket2::~CSocket2()
{
  if(shutdown(iSocket,0)<0)
  {
    iprintf("shutdown error: %d\n",errno); Panic();
  }
  for(size_t ii=0;ii<60;++ii) swiWaitForVBlank();
  if(closesocket(iSocket)<0)
  {
    iprintf("closesocket error: %d\n",errno); Panic();
  }
}

void CSocket2::Bind(unsigned short aPort)
{
  sockaddr_in sa;
  memset(&sa,0,sizeof(sa));
  sa.sin_family=AF_INET;
  sa.sin_addr.s_addr=htonl(INADDR_ANY);
  sa.sin_port=htons(aPort);
  if(bind(iSocket,(const sockaddr*)&sa,sizeof(sa))<0)
  {
    iprintf("bind error\n");
    Panic();
  }
}

void CSocket2::Connect(const char* anAddress,unsigned short aPort)
{
  sockaddr_in sa;
  memset(&sa,0,sizeof(sa));
  sa.sin_family=AF_INET;
  if(!inet_aton(anAddress,&(sa.sin_addr)))
  {
    iprintf("inet_aton error: %d\n",errno);
    Panic();
  }
  sa.sin_port=htons(aPort);
  iprintf("%lx:%x\n",sa.sin_addr.s_addr,sa.sin_port);
  if(connect(iSocket,(const sockaddr*)&sa,sizeof(sa))<0)
  {
    iprintf("connect error: %d\n",errno);
    Panic();
  }
}

void CSocket2::Listen(void)
{
  if(listen(iSocket,1)<0)
  {
    iprintf("listen error\n");
    Panic();
  }
}

CSocket2* CSocket2::Accept(void)
{
  sockaddr_in clientAddr;
  int clientAddrLen=sizeof(clientAddr);
  int newSocket=-1;
  while(true)
  {
    newSocket=accept(iSocket,(sockaddr*)&clientAddr,&clientAddrLen);
    if(newSocket<0)
    {
      if(errno==EAGAIN)
      {
        swiWaitForVBlank();
        continue;
      }
      iprintf("accept error: %d, %d\n",newSocket,errno);
      Panic();
    }
    else break;
  }
  CSocket2* result=new CSocket2(newSocket,iNonBlock);
  return result;
}

int CSocket2::Receive(char* aBuffer,int aSize,bool aWait)
{
  int lenb;
  while(true)
  {
    lenb=recv(iSocket,aBuffer,aSize,0);
    if(lenb<0)
    {
      if(errno==EAGAIN)
      {
        if(aWait)
        {
          swiWaitForVBlank();
        }
        else
        {
          lenb=0;
          break;
        }
        continue;
      }
      iprintf("recv error: %d, %d\n",lenb,errno);
      Panic();
    }
    else break;
  }
  return lenb;
}

int CSocket2::Send(const char* aBuffer)
{
  return Send(aBuffer,strlen(aBuffer),NULL,NULL);
}

int CSocket2::Send(const char* aBuffer,int aSize,TSendCallback aCallBack,void *aParam)
{
  int total=0;
  do
  {
    int sended;
    while(true)
    {
      sended=send(iSocket,aBuffer+total,aSize-total,0);
      if(sended<0)
      {
        if(aCallBack)
        {
          if(!aCallBack(aParam)) return -1;
        }
        if(errno==EAGAIN)
        {
          //iprintf("eagain error\n");
          //swiWaitForVBlank();
          continue;
        }
        iprintf("send error: %d, %d\n",sended,errno);
        Panic();
      }
      else break;
    }
    if(sended==0) break;
    total+=sended;
    //iprintf("sended: %d(%d)\n",sended,total);
  } while(total<aSize);
  return total;
}

void CSocket2::SetNonBlock(bool aNonBlock)
{
  iNonBlock=aNonBlock;
  unsigned long arg=iNonBlock?1:0;
  if(ioctl(iSocket,FIONBIO,&arg))
  {
    closesocket(iSocket);
    iprintf("ioctl error\n");
    Panic();
  }
}
