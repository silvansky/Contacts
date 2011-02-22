#ifndef STYLEVIEWER_H
#define STYLEVIEWER_H

#include <QWebView>
#include <webpage.h>

class StyleViewer :
		public QWebView
{
	Q_OBJECT
public:
	StyleViewer(QWidget *AParent);
	~StyleViewer();
public:
	virtual QSize sizeHint() const;
protected slots:
	void onShortcutActivated();
	void onPageLoaded();
signals:
	void htmlChanged(QWidget *, const QString &);
};

#endif // STYLEVIEWER_H
