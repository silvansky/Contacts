#ifndef ADDACCOUNTWIDGET_H
#define ADDACCOUNTWIDGET_H

#include <QWidget>
#include "serverapihandler.h"

namespace Ui {
class AddAccountWidget;
}

enum AccountWidgetType
{
	AW_Facebook,
	AW_Vkontakte,
	AW_MRIM,
	AW_ICQ,
	AW_Yandex,
	AW_Rambler
};

class AddAccountWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString serviceName READ serviceName WRITE setServiceName)
	Q_PROPERTY(QImage serviceIcon READ serviceIcon WRITE setServiceIcon)
	Q_PROPERTY(AccountWidgetType type READ type)
public:
	AddAccountWidget(AccountWidgetType accWidgetType, QWidget *parent = 0);
	virtual ~AddAccountWidget();
	ServiceAuthInfo authInfo() const;
	// props
	QString serviceName() const;
	void setServiceName(const QString &newName);
	QImage serviceIcon() const;
	void setServiceIcon(const QImage &newIcon);
	AccountWidgetType type() const;
protected slots:
	void onServiceButtonToggled(bool on);
	void onDialogAccepted();
signals:
	void authChecked();
	void authCheckFailed();
private:
	Ui::AddAccountWidget *ui;
private:
	// props
	QString _serviceName;
	QImage _serviceIcon;
	AccountWidgetType _type;
	ServiceAuthInfo _authInfo;
};

#endif // ADDACCOUNTWIDGET_H
