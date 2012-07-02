#ifndef EASYREGISTRATIONDIALOG_H
#define EASYREGISTRATIONDIALOG_H

#include <QDialog>

#include <utils/jid.h>

namespace Ui {
class EasyRegistrationDialog;
}

class EasyRegistrationDialog : public QDialog
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
	virtual void keyPressEvent(QKeyEvent *ke);
protected:
	void startLoading();
	void setError(bool on);
	void parseSize(const QString &sizeString);
	void updateWindowSize();
protected slots:
	void onWindowAnimationComplete();
	void onLoaded(bool ok);
	void onWebPageLinkClicked(const QUrl &url);
private:
	Ui::EasyRegistrationDialog *ui;
private:
	Jid userJid;
	QSize recommendedWebViewSize;
	bool errorSet;
	bool loaderShown;
};

#endif // EASYREGISTRATIONDIALOG_H
