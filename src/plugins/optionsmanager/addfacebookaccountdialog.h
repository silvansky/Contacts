#ifndef ADDFACEBOOKACCOUNTDIALOG_H
#define ADDFACEBOOKACCOUNTDIALOG_H

#include <QDialog>
#include <QUrl>

namespace Ui {
class AddFacebookAccountDialog;
}

class AddFacebookAccountDialog : public QDialog
{
	Q_OBJECT
	Q_PROPERTY(bool succeeded READ succeeded)
	Q_PROPERTY(QString selectedUserId READ selectedUserId)
	Q_PROPERTY(QString selectedUserDisplayName READ selectedUserDisplayName)
	Q_PROPERTY(QString authToken READ authToken)
public:
	explicit AddFacebookAccountDialog(QWidget *parent = 0);
	~AddFacebookAccountDialog();
	void showDialog();
	// props
	bool succeeded() const;
	QString selectedUserId() const;
	QString selectedUserDisplayName() const;
	QString authToken() const;
protected:
	void abort(const QString &message);
	void setWaitMode(bool wait, const QString &message = QString::null);
protected slots:
	void onAdjustWindowSize();
	void onWebViewLoadStarted();
	void onWebViewLoadFinished(bool ok);
	void onWebPageLinkClicked(const QUrl &url);
private:
	Ui::AddFacebookAccountDialog *ui;
	// props
	QString _selectedUserId;
	QString _selectedUserDisplayName;
	QString _authToken;
	bool _succeeded;
};

#endif // ADDFACEBOOKACCOUNTDIALOG_H
