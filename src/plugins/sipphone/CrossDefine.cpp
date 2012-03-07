#include "CrossDefine.h"
#include <qtime>

#ifdef Q_WS_WIN32
# include "winsock2.h"
#endif
//#include "textedit.h"

//#define WINDOWS_BLABLABLA

//#ifdef WINDOWS_BLABLABLA

int gettimeofday(struct timeval *tv, void *tzp)
{
	Q_UNUSED(tzp);
  if(tv == NULL) return -1;

  QTime time = QTime::currentTime();
  tv->tv_sec = time.second();
  tv->tv_usec = time.msec() * 1000;
  return 0;
}


//static TextEdit* debugConsole = NULL;

void dbgPrintf(const char* format)
{
	Q_UNUSED(format);

  //if(debugConsole == NULL)
  //{
  //  debugConsole = new TextEdit();
  //}
  ////if(debugConsole == NULL)
  //if(!debugConsole->isEnabled())
  //{
  //  //debugConsole = new TextEdit();
  //  debugConsole->resize( 1024, 768 );
  //  debugConsole->show();
  //}

  //if(debugConsole->isEnabled())
  //{
  //  debugConsole->printSomethingSimple(format);
  //}

}

void dbgPrintf(const char* format, ...)
{
	Q_UNUSED(format);
  ////#ifndef QT_NO_DEBUG
  //if(debugConsole == NULL)
  //{
  //  debugConsole = new TextEdit();
  //}
  ////if(debugConsole == NULL)
  //if(!debugConsole->isEnabled())
  //{
  //  //debugConsole = new TextEdit();
  //  debugConsole->resize( 1024, 768 );
  //  debugConsole->show();
  //}
  //

  //if(debugConsole->isEnabled())
  //{
  //  va_list args;
  //  va_start(args, format);
  //  debugConsole->printSomethingVa(format, args);
  //  va_end(args);
  //}
//#endif
}
//#endif
