#ifndef NETWORKING_H
#define NETWORKING_H

#include "utilsexport.h"
#include <QUrl>
#include <QImage>

class UTILS_EXPORT Networking
{
public:
	static QImage httpGetImage(const QUrl& src);
	static bool insertPixmap(const QUrl& src, QObject* target, const QString& property = "pixmap");
	static QString httpGetString(const QUrl& src);
private:
	class NetworkingPrivate;
	static NetworkingPrivate networkingPrivate;
};

#endif // NETWORKING_H
