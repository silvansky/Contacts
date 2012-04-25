#ifndef VIDEOLAYOUT_H
#define VIDEOLAYOUT_H

#include <QLayout>
#include <QWidget>

class VideoLayout : 
	public QLayout
{
	Q_OBJECT;
public:
	VideoLayout(QWidget *ARemote, QWidget *ALocal, QWidget *AParent);
	~VideoLayout();
	// VideoLayout
	// QLayout
	virtual int count() const;
	virtual void addItem(QLayoutItem *AItem);
	virtual QLayoutItem *itemAt(int AIndex) const;
	virtual QLayoutItem *takeAt(int AIndex);
	// QLayoutItem
	virtual QSize sizeHint() const;
	virtual void setGeometry(const QRect &ARect);
private:
	QWidget *FLocal;
	QWidget *FRemote;
	QWidget *FControlls;
};

#endif // VIDEOLAYOUT_H
