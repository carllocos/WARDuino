#ifndef SOCKSERVER_H
#define SOCKSERVER_H

#include <FreeRTOS.h>
#include "AsyncTCP.h"

class WARDuino;

class SocketServer {
  private:
	AsyncServer _server = AsyncServer(80);
	WARDuino *_warduino;
  public:
	void setWARDuino(WARDuino *);
	void begin(uint16_t port);
};

#include "WARDuino.h"


#endif