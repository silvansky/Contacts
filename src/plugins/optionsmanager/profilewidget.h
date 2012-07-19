#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include <utils/jid.h>

#include <QWidget>

namespace Ui {
class ProfileWidget;
}

class ProfileWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(Jid jid READ jid WRITE setJid)
	Q_PROPERTY(QImage photo READ photo WRITE setPhoto)
public:
	explicit ProfileWidget(QWidget *parent = 0);
	virtual ~ProfileWidget();
	// props
	QString name() const;
	void setName(const QString &newName);
	Jid jid() const;
	void setJid(const Jid &newJid);
	QImage photo() const;
	void setPhoto(const QImage &newPhoto);
signals:
	void clicked();
	void removeButtonClicked();
protected:
	void paintEvent(QPaintEvent *pe);
	void mousePressEvent(QMouseEvent *me);
private:
	Ui::ProfileWidget *ui;
private:
	// props
	QString _name;
	Jid _jid;
	QImage _photo;
};

#endif // PROFILEWIDGET_H
