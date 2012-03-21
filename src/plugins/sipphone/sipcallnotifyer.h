#ifndef SIPCALLNOTIFYER_H
#define SIPCALLNOTIFYER_H

#include <QWidget>
#ifdef QT_PHONON_LIB
# include <Phonon/Phonon>
#else
# include <QSound>
#endif

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
	void startSound();
	void muteSound();
protected slots:
	void acceptClicked();
	void rejectClicked();
	void muteClicked();
#ifdef QT_PHONON_LIB
	void loopPlay();
#endif
protected:
	void paintEvent(QPaintEvent *);
private:
	Ui::SipCallNotifyer *ui;
private:
	bool _muted;
	QString soundFile;
#ifdef QT_PHONON_LIB
	Phonon::MediaObject *FMediaObject;
	Phonon::AudioOutput *FAudioOutput;
#else
	QSound *FSound;
#endif
};

#endif // SIPCALLNOTIFYER_H
