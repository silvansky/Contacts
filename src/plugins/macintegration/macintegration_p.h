#ifndef MACINTEGRATION_P_H
#define MACINTEGRATION_P_H

#include <QImage>
#include <QColor>

#ifndef COCOA_CLASSES_DEFINED
class NSImage;
class NSString;
#endif

class QImage;
class QTimer;

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
	static void release();
signals:
	void dockClicked();
	void growlNotifyClicked(int);
public slots:
	void startDockAnimation();
	void startDockAnimation(const QImage & imageToRotate, Qt::Alignment align = Qt::AlignCenter);
	void startDockAnimation(QList<QImage> imageSequence, Qt::Alignment align = Qt::AlignCenter);
	void stopDockAnimation();
public:
	bool isDockAnimationRunning() const;
	void emitClick();
	void emitGrowlNotifyClick(int id);
	// static
	static void setDockBadge(const QString & badgeText);
	static void setDockOverlay(const QImage & overlay, Qt::Alignment align = Qt::AlignLeft | Qt::AlignBottom, bool showAppIcon = true);
	static void postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id);
	static void showGrowlPrefPane();
	static bool isGrowlInstalled();
	static bool isGrowlRunning();
	static void installCustomFrame();
	static void setCustomBorderColor(const QColor & color);
	static void setCustomTitleColor(const QColor & color);
	static void setWindowMovableByBackground(QWidget * window, bool movable);
	static void requestAttention();
	static void checkForUpdates();
protected slots:
	void onUpdateTimer();
private:
	QTimer * updateTimer;
	// statics
	static MacIntegrationPrivate * _instance;
};

#endif // MACINTEGRATION_P_H
