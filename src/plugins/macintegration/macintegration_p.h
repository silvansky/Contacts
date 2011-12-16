#ifndef MACINTEGRATION_P_H
#define MACINTEGRATION_P_H

#include <QObject>
#include <QColor>

#ifndef COCOA_CLASSES_DEFINED
class NSImage;
class NSString;
#endif

class QImage;

// private class
class MacIntegrationPrivate : public QObject
{
	Q_OBJECT
	friend class MacIntegrationPlugin;
private:
	MacIntegrationPrivate();
public:
	~MacIntegrationPrivate();
	static MacIntegrationPrivate * instance();
	static NSImage * nsImageFromQImage(const QImage & image);
	static QImage qImageFromNSImage(NSImage * image);
	static NSString * nsStringFromQString(const QString & s);
signals:
	void dockClicked();
	void growlNotifyClicked(int);
public:
	void emitClick();
	void emitGrowlNotifyClick(int id);
	// static
	static void setDockBadge(const QString & badgeText);
	static void postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id);
	static void showGrowlPrefPane();
	static void installCustomFrame();
	static void setCustomBorderColor(const QColor & color);
	static void setCustomTitleColor(const QColor & color);
	static void setWindowMovableByBackground(QWidget * window, bool movable);
	static void requestAttention();
private:
	static MacIntegrationPrivate * _instance;
};

#endif // MACINTEGRATION_P_H
