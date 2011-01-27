#ifndef CUSTOMBORDERCONTAINER_P_H
#define CUSTOMBORDERCONTAINER_P_H

/*
  NOTE: this file shouldn't be included directly anywhere outside utils. Use customcorderstorage.h.
*/

#include "borderstructs.h"
#include "custombordercontainer.h"
#include <QDomDocument>
#include <QFile>
#include <QMultiMap>

class CustomBorderContainerPrivate
{
	friend class CustomBorderContainer;
public:
	CustomBorderContainerPrivate(CustomBorderContainer * parent);
	CustomBorderContainerPrivate(const CustomBorderContainerPrivate&);
	~CustomBorderContainerPrivate();
	void parseFile(const QString & fileName);
private:
	void setAllDefaults();
	QColor parseColor(const QString & name);
	QGradient * parseGradient(const QDomElement & element);
	ImageFillingStyle parseImageFillingStyle(const QString & style);
	void setDefaultBorder(Border & border);
	void parseBorder(const QDomElement & borderElement, Border & border);
	void setDefaultCorner(Corner & corner);
	void parseCorner(const QDomElement & cornerElement, Corner & corner);
	void setDefaultHeader(Header & header);
	void parseHeader(const QDomElement & headerElement, Header & header);
	void setDefaultHeaderTitle(HeaderTitle & title);
	void parseHeaderTitle(const QDomElement & titleElement, HeaderTitle & title);
	void setDefaultWindowIcon(WindowIcon & windowIcon);
	void parseWindowIcon(const QDomElement & iconElement, WindowIcon & windowIcon);
	void setDefaultWindowControls(WindowControls & windowControls);
	void parseWindowControls(const QDomElement & controlsElement, WindowControls & windowControls);
	void setDefaultHeaderButton(HeaderButton & button);
	void parseHeaderButton(const QDomElement & buttonElement, HeaderButton & button);
protected:
	CustomBorderContainer * p;
	Corner topLeft, topRight, bottomLeft, bottomRight;
	Border left, right, top, bottom;
	Header header;
	HeaderTitle title;
	WindowIcon icon;
	WindowControls controls;
	HeaderButton minimize, maximize, close, restore;
	QMultiMap<int, HeaderButton> headerButtons;
};

#endif // CUSTOMBORDERCONTAINER_P_H
