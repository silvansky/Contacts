#ifndef EASYREGISTRATIONDIALOG_H
#define EASYREGISTRATIONDIALOG_H

#include <QWidget>
#include <utils/jid.h>

namespace Ui {
class EasyRegistrationDialog;
}

class EasyRegistrationDialog : public QWidget
{
	Q_OBJECT
	
public:
	explicit EasyRegistrationDialog(QWidget *parent = 0);
	~EasyRegistrationDialog();
signals:
	void aborted();
	void registered(const Jid &user);
protected:
	virtual void showEvent(QShowEvent *se);
	virtual void closeEvent(QCloseEvent *ce);
protected:
	void startLoading();
protected slots:
	void onLoaded(bool ok);
private:
	Ui::EasyRegistrationDialog *ui;
private:
	Jid userJid;
};

#endif // EASYREGISTRATIONDIALOG_H
