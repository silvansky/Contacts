#ifndef SIPCALLNOTIFYER_H
#define SIPCALLNOTIFYER_H

#include <QWidget>

class CustomBorderContainer;

namespace Ui {
class SipCallNotifyer;
}

class SipCallNotifyer : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(double opacity READ opacity WRITE setOpacity)
public:
	SipCallNotifyer(const QString & caption, const QString & notice, const QIcon & icon, const QImage & avatar);
	~SipCallNotifyer();
	bool isMuted() const;
	double opacity() const;
	void setOpacity(double op);
signals:
	void accepted();
	void rejected();
	void muted();
	void unmuted();
public slots:
	void appear();
	void disappear();
protected slots:
	void acceptClicked();
	void rejectClicked();
	void muteClicked();
protected:
	void paintEvent(QPaintEvent *);
private:
	Ui::SipCallNotifyer *ui;
	CustomBorderContainer * border;
	bool _muted;
};

#endif // SIPCALLNOTIFYER_H
