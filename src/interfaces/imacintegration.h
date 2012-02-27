#ifndef IMACINTEGRATION_H
#define IMACINTEGRATION_H

#include <QObject>
#include <QMenuBar>
#include <utils/menu.h>

#define MACINTEGRATION_UUID "{c936122e-cbf3-442d-b01f-8ecc5b0ad530}"

class IMacIntegration
{
public:
	virtual QObject * instance() = 0;
	virtual Menu * dockMenu() = 0;
	virtual QMenuBar * menuBar() = 0;
	virtual Menu * fileMenu() = 0;
	virtual Menu * editMenu() = 0;
	virtual Menu * viewMenu() = 0;
	virtual Menu * statusMenu() = 0;
	virtual Menu * windowMenu() = 0;
	virtual Menu * helpMenu() = 0;
	virtual void setDockBadge(const QString & badgeText) = 0;
	virtual void setDockOverlayImage(const QImage & image, Qt::Alignment alignment = Qt::AlignCenter, bool showAppIcon = true) = 0;
	virtual void requestUserAttention() = 0;
	virtual void postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id) = 0;
	virtual void showGrowlPreferencePane() = 0;
	virtual void setCustomBorderColor(const QColor & color) = 0;
	virtual void setCustomTitleColor(const QColor & color) = 0;
	virtual void setWindowMovableByBackground(QWidget * window, bool movable) = 0;
	virtual void startDockAnimation() = 0;
	virtual void startDockAnimation(const QImage & imageToRotate, Qt::Alignment align = Qt::AlignCenter) = 0;
	virtual void startDockAnimation(QList<QImage> imageSequence, Qt::Alignment align = Qt::AlignCenter) = 0;
	virtual void stopDockAnimation() = 0;
	virtual bool isDockAnimationRunning() const = 0;
protected:
	// signals
	virtual void dockClicked() = 0;
	virtual void growlNotifyClicked(int id) = 0;
};

Q_DECLARE_INTERFACE(IMacIntegration,"Virtus.Core.IMacIntegration/1.0")

#endif // IMACINTEGRATION_H
