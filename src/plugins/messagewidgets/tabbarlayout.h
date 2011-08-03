#ifndef TABBARLAYOUT_H
#define TABBARLAYOUT_H

#include <QList>
#include <QLayout>

class TabBarLayout :
	public QLayout
{
	Q_OBJECT
public:
	TabBarLayout(QWidget *AParent = NULL);
	virtual ~TabBarLayout();
	// TabBarLayout
	int minimumItemWidth() const;
	int maximumItemWidth() const;
	void setMinMaxItemWidth(int AMin, int AMax);
	void blockUpdate(bool ABlock);
	void updateLayout();
	int indexToOrder(int AIndex) const;
	int orderToIndex(int AOrder) const;
	void moveItem(int ATarget, int ADestination);
	// QLayout
	virtual int count() const;
	virtual void addItem(QLayoutItem *AItem);
	virtual QLayoutItem *itemAt(int AIndex) const;
	virtual QLayoutItem *takeAt(int AIndex);
	// QLayoutItem
	virtual QSize sizeHint() const;
	virtual bool hasHeightForWidth() const;
	virtual int heightForWidth(int AWidth) const;
	virtual Qt::Orientations expandingDirections() const;
	virtual void setGeometry(const QRect &ARect);
protected:
	void calcLayoutParams(int AWidth, int &AItemWidth, bool &AStreatch) const;
	int doLayout(QRect ARect, int AItemWidth, bool AStreatch, bool AResize) const;
private:
	int FMinWidth;
	int FMaxWidth;
	int FItemsWidth;
	bool FStreatch;
	bool FUpdateBlocked;
	QList<QLayoutItem *> FItems;
	QList<QLayoutItem *> FItemsOrder;
};

#endif // TABBARLAYOUT_H
