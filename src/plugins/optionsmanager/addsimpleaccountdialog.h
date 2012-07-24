#ifndef ADDSIMPLEACCOUNTDIALOG_H
#define ADDSIMPLEACCOUNTDIALOG_H

#include "serverapihandler.h"

#include <utils/menu.h>

#include <QDialog>

namespace Ui {
class AddSimpleAccountDialog;
}

class AddSimpleAccountDialog : public QDialog
{
	Q_OBJECT
	// props
	Q_PROPERTY(QString service READ service WRITE setService)
	Q_PROPERTY(QString caption READ caption WRITE setCaption)
	Q_PROPERTY(QImage icon READ icon WRITE setIcon)
	Q_PROPERTY(QString loginPlaceholder READ loginPlaceholder WRITE setLoginPlaceholder)
	Q_PROPERTY(QString passwordPlaceholder READ passwordPlaceholder WRITE setPasswordPlaceholder)
	Q_PROPERTY(QStringList domainList READ domainList WRITE setDomainList)
	Q_PROPERTY(bool succeeded READ succeeded)
	Q_PROPERTY(QString selectedUserId READ selectedUserId)
	Q_PROPERTY(QString selectedUserDisplayName READ selectedUserDisplayName)
	Q_PROPERTY(QString authToken READ authToken)
public:
	AddSimpleAccountDialog();
	virtual ~AddSimpleAccountDialog();
	void showDialog();
	// props
	QString service() const;
	void setService(const QString &newService);
	QString caption() const;
	void setCaption(const QString &newCaption);
	QImage icon() const;
	void setIcon(const QImage &newIcon);
	QString loginPlaceholder() const;
	void setLoginPlaceholder(const QString &newPlaceholder);
	QString passwordPlaceholder() const;
	void setPasswordPlaceholder(const QString &newPlaceholder);
	QStringList domainList() const;
	void setDomainList(const QStringList &newList);
	bool succeeded() const;
	QString selectedUserId() const;
	QString selectedUserDisplayName() const;
	QString authToken() const;
protected slots:
	void onDomainActionTriggered();
	void onAdjustWindowSize();
	void onAcceptClicked();
protected slots:
	// for server api
	void onCheckAuthRequestSucceeded(const QString &user, const QString &displayName, const QString &authToken_);
	void onCheckAuthRequestFailed(const QString &user, const QString &reason);
	void onCheckAuthRequestRequestFailed(const QString &error);
private:
	Ui::AddSimpleAccountDialog *ui;
private:
	// props
	QString _service;
	QString _caption;
	QImage _icon;
	QString _loginPlaceholder;
	QString _passwordPlaceholder;
	QStringList _domainList;
	QString _selectedUserId;
	QString _selectedUserDisplayName;
	QString _authToken;
	bool _succeeded;
private:
	Menu *domainsMenu;
	ServerApiHandler *_serverApiHandler;
};

#endif // ADDSIMPLEACCOUNTDIALOG_H
