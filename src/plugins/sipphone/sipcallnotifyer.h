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

public:
	SipCallNotifyer(const QString & caption, const QString & notice, const QIcon & icon, const QImage & avatar);
	~SipCallNotifyer();
	bool isMuted() const;
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
	void animationValueChanged(const QVariant & value);
private:
	Ui::SipCallNotifyer *ui;
	CustomBorderContainer * border;
	bool _muted;
};

#endif // SIPCALLNOTIFYER_H
