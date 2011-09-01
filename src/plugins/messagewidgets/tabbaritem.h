#ifndef TABBARITEM_H
#define TABBARITEM_H

#include <QSize>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imessagewidgets.h>
#include <utils/closebutton.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/customlabel.h>

class TabBarItem :
	public QFrame
{
	Q_OBJECT
	Q_PROPERTY(bool isActive READ isActive WRITE setActive)
	Q_PROPERTY(bool isDraging READ isDraging WRITE setDraging)
	Q_PROPERTY(bool isCloseable READ isCloseable WRITE setCloseable)
	Q_PROPERTY(bool left READ isLeft WRITE setLeft)
	Q_PROPERTY(bool right READ isRight WRITE setRight)
	Q_PROPERTY(bool top READ isTop WRITE setTop)
	Q_PROPERTY(bool bottom READ isBottom WRITE setBottom)
public:
	TabBarItem(QWidget *AParent);
	virtual ~TabBarItem();
	bool isActive() const;
	void setActive(bool AActive);
	bool isDraging() const;
	void setDraging(bool ADraging);
	bool isCloseable() const;
	void setCloseable(bool ACloseable);
	QSize iconSize() const;
	void setIconSize(const QSize &ASize);
	QIcon icon() const;
	void setIcon(const QIcon &AIcon);
	QString iconKey() const;
	void setIconKey(const QString &AIconKey);
	QString text() const;
	void setText(const QString &AText);
	QString toolTip() const;
	void setToolTip(const QString &AToolTip);
	ITabPageNotify notify() const;
	void setNotify(const ITabPageNotify &ANotify);
	// left-right-top-bottom props
	bool isLeft() const;
	void setLeft(bool on);
	bool isRight() const;
	void setRight(bool on);
	bool isTop() const;
	void setTop(bool on);
	bool isBottom() const;
	void setBottom(bool on);
	// changed flag
	bool isChanged() const;
	void setChanged(bool c);
signals:
	void closeButtonClicked();
protected:
	void showIcon(const QIcon &AIcon);
	void showIconKey(const QString &AIconKey, const QString &AIconStorage);
	void showText(const QString &AText);
	void showToolTip(const QString &AToolTip);
	void showStyleKey(const QString &AStyleKey);
protected:
	virtual void paintEvent(QPaintEvent *AEvent);
	virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onBlinkTimerTimeout();
private:
	QLabel *FIconLabel;
	CustomLabel *FTextLabel;
	CloseButton *FCloseButton;
private:
	bool FActive;
	bool FDraging;
	QIcon FIcon;
	QSize FIconSize;
	QString FIconKey;
	QString FText;
	QString FToolTip;
private:
	bool FIconHidden;
	QTimer FBlinkTimer;
	ITabPageNotify FNotify;
	bool left, right, top, bottom;
	bool changed;
};

#endif // TABBARITEM_H
