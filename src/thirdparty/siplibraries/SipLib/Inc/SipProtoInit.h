#ifndef SIPPROTOINIT_H_INCLUDED
#define SIPPROTOINIT_H_INCLUDED

#include "sipprotocol_global.h"

class SIPPROTOCOL_EXPORT SipProtoInit
{
public:
  static int Init();

public:
  static int ListenSipPort() { return _listenSipPort; }
  static void SetListenSipPort(int val) { _listenSipPort = val; }

  static int ProxySipPort() { return _proxySipPort; }
  static void SetProxySipPort(int val) { _proxySipPort = val; }

private:
  static int _listenSipPort;
  static int _proxySipPort;
};

#endif // SIPPROTOINIT_H_INCLUDED
