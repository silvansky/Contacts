#ifndef CUSTOMLISTVIEW_H
#define CUSTOMLISTVIEW_H

#include <QListView>
#include "utilsexport.h"

class CustomBorderContainer;

class UTILS_EXPORT CustomListView : public QListView
{
	Q_OBJECT
public:
	CustomListView();
protected:
	bool event(QEvent *);
private:
	CustomBorderContainer * border;
};

#endif // CUSTOMLISTVIEW_H
