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
#include <dswifi9.h>
#include <netinet/in.h>
#include "socket2.h"
#include "card_access.h"

CCard* card=NULL;
char line[256];

bool check(const char* aBuffer,const char* aCommand)
{
  return strncmp(aBuffer,aCommand,strlen(aCommand))==0;
}

void process(CSocket2* aConn)
{
  aConn->Send("220 Cthulhu ready\r\n");
  char buffer[1024];
  char address[44]="";
  unsigned short port=0;
  while(true)
  {
    int size=aConn->Receive(buffer,1023);
    buffer[size]=0;
    iprintf(buffer);
    if(size==0) break;
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
      aConn->Send("200 Type set to I\r\n");
    }
    else if(check(buffer,"PORT"))
    {
      u32 h1,h2,h3,h4,p1,p2;
      sscanf(buffer+5,"%3u,%3u,%3u,%3u,%3u,%3u",&h1,&h2,&h3,&h4,&p1,&p2);
      //iprintf("port: %d.%d.%d.%d:%d\n",h1,h2,h3,h4,p1+p2*256);
      sprintf(address,"%u.%u.%u.%u",h1,h2,h3,h4);
      port=((p1&0xff)<<8)|(p2&0xff);
      iprintf("address: %s\nport: %d\n",address,port);
      aConn->Send("200 PORT command successful\r\n");
    }
    else if(check(buffer,"LIST"))
    {
      aConn->Send("150 Opening connection\r\n");
      CSocket2* list=new CSocket2(false);
      list->Connect(address,port);
      //list->SetBlock(true);
      //list->Send("-rw-r--r--    1 root       root             4096 Jan  1  2004 xxxx\r\n");

      //char buffer[128];
      //sprintf(buffer,"-rw-r--r--    1 root       root       %10u Jan  1  2004 xxxx\r\n",card->SecureAreaSize());
      //list->Send(buffer);

      sprintf(line,"-rw-r--r--    1 root       root       %10u Jan  1  2004 %s.nds\r\n",card->CartSize(),card->Name());
      list->Send(line);
      //list->Send("-rw-r--r--    1 root       root            32768 Jan  1  2004 xxxx\r\n");
      delete list;
      aConn->Send("226 Transfer Complete\r\n");
    }
    else if(check(buffer,"RETR"))
    {
      aConn->Send("150 Opening BINARY mode data connection\r\n");
      CSocket2* data=new CSocket2(false);
      data->Connect(address,port);
      //data->SetBlock(true);
      //for(size_t ii=0;ii<256;++ii)
        //data->Send((const char*)gData,sizeof(gData));
      u32 size=card->CartSize(),pos=0,block=card->BufferSize();
      data->Send((const char*)card->SecureArea(),card->SecureAreaSize());
      pos+=card->SecureAreaSize();
      while(pos<size)
      {
        const char* ptr=(const char*)card->Buffer(pos);
        data->Send(ptr,block);
        pos+=card->BufferSize();
      }
      delete data;
      aConn->Send("226 Transfer Complete\r\n");
    }
    else if(check(buffer,"QUIT"))
    {
      aConn->Send("221 goodbye\r\n");
      break;
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
    CSocket2* server=new CSocket2(false);
    server->Bind(21);
    iprintf("listen\n");
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
