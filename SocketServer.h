#ifndef SOCKSERVER_H
#define SOCKSERVER_H

#include <FreeRTOS.h>
#include "AsyncTCP.h"

class WARDuino;

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

#include "WARDuino.h"


#endif
