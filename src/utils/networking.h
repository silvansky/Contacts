#ifndef NETWORKING_H
#define NETWORKING_H

#include "utilsexport.h"
#include <QUrl>
#include <QImage>

class NetworkingPrivate;

// slots for *Async methods should be like this:
// void onGetString(const QUrl &url, const QString &result);
// void onNetworkError(const QUrl &url, const QString &errorString);
// the "slot" string must be the name of slot only
// you can use NW_SLOT(function) macro
// use NULL (or NW_SLOT_NONE) to ignore result

#define NW_SLOT(func)    #func
#define NW_SLOT_NONE     NULL

class UTILS_EXPORT Networking
{
public:
	static QImage httpGetImage(const QUrl &src);
	static void httpGetImageAsync(const QUrl &src, QObject *receiver, const char *slot, const char *errorSlot);
	static bool insertPixmap(const QUrl &src, QObject *target, const QString &property = "pixmap");
	static QString httpGetString(const QUrl &src);
	static void httpGetStringAsync(const QUrl &src, QObject *receiver, const char *slot, const char *errorSlot);
	static void httpPostAsync(const QUrl &src, const QByteArray &data, QObject *receiver, const char *slot, const char *errorSlot);
	static QString cookiePath();
	static void setCookiePath(const QString &path);
private:
	static NetworkingPrivate *networkingPrivate;
	static NetworkingPrivate * p();
};

#endif // NETWORKING_H
