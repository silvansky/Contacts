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
	Q_PROPERTY(bool isLeft READ isLeft WRITE setLeft)
	Q_PROPERTY(bool isRight READ isRight WRITE setRight)
	Q_PROPERTY(bool isTop READ isTop WRITE setTop)
	Q_PROPERTY(bool isBottom READ isBottom WRITE setBottom)
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
	void setLeft(bool ALeft);
	bool isRight() const;
	void setRight(bool ARight);
	bool isTop() const;
	void setTop(bool ATop);
	bool isBottom() const;
	void setBottom(bool ABottom);
signals:
	void closeButtonClicked();
protected:
	void showIcon(const QIcon &AIcon);
	void showIconKey(const QString &AIconKey, const QString &AIconStorage);
	void showText(const QString &AText);
	void showToolTip(const QString &AToolTip);
	void showStyleKey(const QString &AStyleKey);
protected:
	void paintEvent(QPaintEvent *AEvent);
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onBlinkTimerTimeout();
	void onUpdateTimerTimeout();
private:
	QLabel *FIconLabel;
	CustomLabel *FTextLabel;
	CloseButton *FCloseButton;
private:
	bool FActive;
	bool FDraging;
	bool FLeft, FRight, FTop, FBottom;
private:
	QIcon FIcon;
	QSize FIconSize;
	QString FIconKey;
	QString FText;
	QString FToolTip;
private:
	bool FIconHidden;
	QTimer FBlinkTimer;
	QTimer FUpdateTimer;
	ITabPageNotify FNotify;
};

#endif // TABBARITEM_H
