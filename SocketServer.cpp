#include <stdio.h>
#include "SocketServer.h"

void registerCallbacks(AsyncClient *, WARDuino *);

void SocketServer::setWARDuino(WARDuino *wd){
  _warduino = wd;
}

void SocketServer::begin(uint16_t port)
  {
    _server.begin();
    _server.onClient([this](void * s, AsyncClient* c){
      printf("A client connected!\n");
      if(c == NULL){
        printf("Client is NULL\n");
        return;
      }
      registerCallbacks(c, _warduino);
    }, NULL);
}

void registerCallbacks(AsyncClient * c, WARDuino *warduino){
  c->onError([](void *r, AsyncClient* c, int8_t error){ 
    printf("Event Error\n");
    }, NULL);
  c->onAck([](void *r, AsyncClient* c, size_t len, uint32_t time){ 
    printf("Ack: time \n");
    }, NULL);
  c->onDisconnect([](void *r, AsyncClient* c){
      printf("Client Disconnected\n");
      c->close(true);
      c->free();
    delete c;
  }, NULL);
  c->onTimeout([](void *r, AsyncClient* c, uint32_t time){
    printf("Client timedout\n");
    }, NULL);
  c->onData([warduino](void *r, AsyncClient* c, void *buf, size_t len){
    printf("got data size\n");
    warduino->handleInterrupt(len, (uint8_t *) buf);
    }, NULL);
  c->onPoll([](void *r, AsyncClient* c){
    (void)c;
    printf("Polling\n");
    }, NULL);
  c->write("ping!\n");
}