#ifdef Arduino
#include <stdio.h>
#include "SocketServer.h"

void SocketServer::registerVM(WARDuino * t_warduino){
  this->_warduino = t_warduino;
}


SocketServer::SocketServer(uint16_t t_port, WARDuino *t_wrd): asyncServer(AsyncServer(t_port)), _warduino(t_wrd)
{
  forIO = nullptr;
  forEvents = nullptr;
  forProxy = nullptr;
}

void SocketServer::begin()
  {
    this->asyncServer.begin();
    this->asyncServer.onClient([this](void * s, AsyncClient* c){
      printf("A client connected!\n");
      if(c == NULL){
        printf("Client is NULL\n");
        return;
      }

      this->registerClient(c);
    }, NULL);
}


void SocketServer::registerClient(AsyncClient *t_client){
  if(this->forIO == nullptr){
    this->forIO = t_client;
  }
  else if(this->forEvents == nullptr){
    this->forEvents = t_client;
  }
  else if(this->forProxy == nullptr){
    this->forProxy = t_client;
  }
  else{
    printf("too much socket clients. Closin!!\n");
    t_client->close(true);
    t_client->free();
    delete t_client;
    return;
  }

  WARDuino *wrd = this->_warduino;
  SocketServer *thisServer = this;
  t_client->onError([thisServer](void *r, AsyncClient* t_client, int8_t error){ 
      printf("Event Error\n");
      thisServer->unregisterClient(t_client);
      }, NULL);
  t_client->onDisconnect([thisServer](void *r, AsyncClient* t_client){
      printf("Client Disconnected\n");
      thisServer->unregisterClient(t_client);
  }, NULL);
  t_client->onTimeout([thisServer](void *r, AsyncClient* t_client, uint32_t time){
      printf("Client timeouted\n");
      thisServer->unregisterClient(t_client);
    }, NULL);
  t_client->onData([wrd](void *r, AsyncClient* t_client, void *buf, size_t len){
    wrd->handleInterrupt(len, (uint8_t *) buf);
    }, NULL);
  /* t_client->onAck([](void *r, AsyncClient* t_client, size_t len, uint32_t time){ */ 
  /*   printf("Ack: time \n"); */
  /*   }, NULL); */
  /* t_client->onPoll([](void *r, AsyncClient* t_client){ (void)c;}, NULL); */
  /* c->write("ping!\n"); */
}

void SocketServer::unregisterClient(AsyncClient * t_client){
  if(this->forIO == t_client){
    this->forIO = nullptr;
  }
  else if(this->forEvents == t_client){
    this->forEvents = nullptr;
  }
  else if(this->forProxy == t_client){
    this->forProxy = nullptr;
  }
  else{
    printf("Unknown client is being closed!\n");
    return;
  }
  t_client->close(true);
  t_client->free();
  delete t_client;
}
#endif

