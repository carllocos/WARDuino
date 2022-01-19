#ifndef SOCKSERVER_H
#define SOCKSERVER_H

#ifdef Arduino
#include <FreeRTOS.h>
#include "AsyncTCP.h"
#include "WARDuino.h"

class SocketServer {
  private:
	  AsyncServer asyncServer;

	  WARDuino *_warduino;

  public:
    AsyncClient *forIO;
    AsyncClient *forEvents;
    AsyncClient *forProxy;

    SocketServer(uint16_t t_port, WARDuino * t_wrd);
    void registerVM(WARDuino * t_warduino);
    void registerClient(AsyncClient *t_client);
    void unregisterClient(AsyncClient *t_client);
	  void begin();
};

#endif
#endif
