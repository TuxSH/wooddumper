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
#include <locale.h>
#include <dswifi9.h>
#include <netinet/in.h>
#include "socket2.h"
#include "card_access.h"

CCard* card=NULL;
char line[256];
char buffer[1024];
char bufferSend[1024];
struct TCallbackData
{
  CSocket2* iConn;
  bool iAbor;
};

bool check(const char* aBuffer,const char* aCommand)
{
  return strncmp(aBuffer,aCommand,strlen(aCommand))==0;
}

bool SendCallback(void *aParam)
{
  TCallbackData* data=(TCallbackData*)aParam;
  u32 controlSize=data->iConn->Receive(bufferSend,1023,false);
  bufferSend[controlSize]=0;
  if(controlSize)
  {
    if(check(bufferSend,"NOOP"))
    {
      data->iConn->Send("200 OK\r\n");
    }
    else //all other commands interpreted as ABOR.
    {
      data->iAbor=true;
      return false;
    }
  }
  return true;
}

void process(CSocket2* aConn)
{
  aConn->Send("220 Cthulhu ready\r\n");
  char address[44]="";
  unsigned short port=0;
  u32 restore=0;
  while(true)
  {
    int size=aConn->Receive(buffer,1023,true);
    buffer[size]=0;
    iprintf(buffer);
    if(size==0) break;
    if(!check(buffer,"RETR"))
    {
      restore=0;
    }
    if(check(buffer,"USER"))
    {
      aConn->Send("331 login accepted\r\n");
    }
    else if(check(buffer,"PASS"))
    {
      aConn->Send("230 logged in\r\n");
    }
    else if(check(buffer,"SYST"))
    {
      aConn->Send("215 UNIX Type: L8\r\n");
    }
    else if(check(buffer,"PWD")||check(buffer,"XPWD"))
    {
      aConn->Send("257 \"/\"\r\n");
    }
    else if(check(buffer,"TYPE"))
    {
      aConn->Send("200 OK\r\n");
    }
    else if(check(buffer,"PORT"))
    {
      u32 h1,h2,h3,h4,p1,p2;
      sscanf(buffer+5,"%3u,%3u,%3u,%3u,%3u,%3u",&h1,&h2,&h3,&h4,&p1,&p2);
      sprintf(address,"%u.%u.%u.%u",h1,h2,h3,h4);
      port=((p1&0xff)<<8)|(p2&0xff);
      aConn->Send("200 PORT command successful\r\n");
    }
    else if(check(buffer,"LIST"))
    {
      aConn->Send("150 Opening connection\r\n");
      CSocket2* list=new CSocket2(false);
      list->Connect(address,port);
      sprintf(line,"-rw-r--r--    1 root       root       %10u Jan  1  2004 %s.nds\r\n",card->CartSize(),card->Name());
      list->Send(line);
      delete list;
      aConn->Send("226 Transfer Complete\r\n");
    }
    else if(check(buffer,"RETR"))
    {
      TCallbackData callbackData={aConn,false};
      aConn->Send("150 Opening BINARY mode data connection\r\n");
      CSocket2* data=new CSocket2(false);
      data->Connect(address,port);
      data->SetNonBlock(true);

      u32 cardSize=card->CartSize(),pos=0,block=card->BufferSize();
      if(card->SecureAreaSize()>restore)
      {
        u32 restore_send=card->SecureAreaSize()-restore;
        u32 sended=data->Send((const char*)(card->SecureArea()+restore),restore_send,SendCallback,&callbackData);
        if(sended!=restore_send)
        {
          if(!callbackData.iAbor)
          {
            iprintf("send error 0\n");
            while(true) swiWaitForVBlank();
          }
        }
        restore=0;
      }
      else
      {
        restore-=card->SecureAreaSize();
      }
      pos+=card->SecureAreaSize();
      if(!callbackData.iAbor)
      {
        if(restore)
        {
          u32 restore_small=restore&(block-1),restore_big=restore-restore_small,restore_send=block-restore_small;
          pos+=restore_big;
          const char* ptr=(const char*)card->Buffer(pos);
          u32 sended=data->Send(ptr+restore_small,restore_send,SendCallback,&callbackData);
          if(sended!=restore_send)
          {
            if(!callbackData.iAbor)
            {
              iprintf("send error 1\n");
              while(true) swiWaitForVBlank();
            }
          }
          pos+=block;
        }
        if(!callbackData.iAbor)
        {
          while(pos<cardSize)
          {
            const char* ptr=(const char*)card->Buffer(pos);
            u32 sended=data->Send(ptr,block,SendCallback,&callbackData);
            if(sended!=block)
            {
              if(callbackData.iAbor) break;
              iprintf("send error 2\n");
              while(true) swiWaitForVBlank();
            }
            pos+=block;
          }
        }
      }
      delete data;
      if(callbackData.iAbor)
      {
        swiWaitForVBlank();
        aConn->Receive(bufferSend,1023,false);
        aConn->Send("426 Connection closed; transfer aborted.\r\n");
        aConn->Send("226 ABOR command successful\r\n");
      }
      else
      {
        aConn->Send("226 Transfer Complete\r\n");
      }
    }
    else if(check(buffer,"QUIT"))
    {
      aConn->Send("221 goodbye\r\n");
      break;
    }
    else if(check(buffer,"REST"))
    {
      sscanf(buffer+5,"%u",&restore);
      iprintf("restore: %u\n",restore);
      sprintf(line,"350 Restarting at %u. Send RETR to initiate transfer\r\n",restore);
      aConn->Send(line);
    }
    else
    {
      aConn->Send("500 command not recognized\r\n");
    }
  }
}

int main(void)
{
  sysSetBusOwners(BUS_OWNER_ARM9,BUS_OWNER_ARM9);
  setlocale(LC_ALL,"C-UTF-8");

  consoleDemoInit();  //setup the sub screen for printing

  card=new CCard;
  card->Init();

  iprintf("Connecting via WFC data ...\n");

  if(!Wifi_InitDefault(WFC_CONNECT))
  {
    iprintf("Failed to connect!");
  }
  else
  {
    iprintf("Connected\n");
    in_addr ip;
    ip=Wifi_GetIPInfo(NULL,NULL,NULL,NULL);
    iprintf("ip: %s\n",inet_ntoa(ip));
    CSocket2* server=new CSocket2(true);
    server->Bind(21);
    server->Listen();
    while(true)
    {
      iprintf("accept\n");
      CSocket2* conn=server->Accept();
      process(conn);
      delete conn;
    }
  }

  return 0;
}
