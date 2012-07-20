#ifndef ADDRAMBLERACCOUNTDIALOG_H
#define ADDRAMBLERACCOUNTDIALOG_H

#include <QDialog>

#include "serverapihandler.h"

namespace Ui {
class AddRamblerAccountDialog;
}

class AddRamblerAccountDialog : public QDialog
{
	Q_OBJECT
	Q_PROPERTY(bool succeeded READ succeeded)
	Q_PROPERTY(QString selectedUserId READ selectedUserId)
	Q_PROPERTY(QString authToken READ authToken)
public:
	explicit AddRamblerAccountDialog(QWidget *parent = 0);
	~AddRamblerAccountDialog();
	void showDialog();
	// props
	bool succeeded() const;
	QString selectedUserId() const;
	QString authToken() const;
protected slots:
	void onAdjustWindowSize();
protected slots:
	// for server api
	void onRegistrationSucceeded(const Jid &user);
	void onRegistrationFailed(const QString &reason, const QString &loginError, const QString &passwordError, const QStringList &suggests);
	void onCheckAuthRequestSucceeded(const QString &user, const QString &authToken_);
	void onCheckAuthRequestFailed(const QString &user, const QString &reason);
	void onCheckAuthRequestRequestFailed(const QString &error);
private:
	Ui::AddRamblerAccountDialog *ui;
	ServerApiHandler *_serverApiHandler;
private:
	// props
	QString _selectedUserId;
	QString _authToken;
	bool _succeeded;
};

#endif // ADDRAMBLERACCOUNTDIALOG_H
