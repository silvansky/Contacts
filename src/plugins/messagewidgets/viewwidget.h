#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H

#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include "ui_viewwidget.h"

class ViewWidget :
	public QWidget,
	public IViewWidget
{
	Q_OBJECT
	Q_INTERFACES(IViewWidget)
public:
	ViewWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
	~ViewWidget();
	virtual QWidget *instance() { return this; }
	virtual const Jid &streamJid() const { return FStreamJid; }
	virtual void setStreamJid(const Jid &AStreamJid);
	virtual const Jid &contactJid() const { return FContactJid; }
	virtual void setContactJid(const Jid &AContactJid);
	virtual QWidget *styleWidget() const;
	virtual IMessageStyle *messageStyle() const;
	virtual void setMessageStyle(IMessageStyle *AStyle, const IMessageStyleOptions &AOptions);
	virtual QUuid changeContentHtml(const QString &AHtml, const IMessageContentOptions &AOptions);
	virtual QUuid changeContentText(const QString &AText, const IMessageContentOptions &AOptions);
	virtual QUuid changeContentMessage(const Message &AMessage, const IMessageContentOptions &AOptions);
	virtual void contextMenuForView(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu);
signals:
	void streamJidChanged(const Jid &ABefour);
	void contactJidChanged(const Jid &ABefour);
	void messageStyleChanged(IMessageStyle *ABefour, const IMessageStyleOptions &AOptions);
	void contentChanged(const QUuid &AContentId, const QString &AMessage, const IMessageContentOptions &AOptions);
	void viewContextMenu(const QPoint &APosition, const QTextDocumentFragment &ASelection, Menu *AMenu);
	void urlClicked(const QUrl &AUrl) const;
protected:
	void initialize();
	QString getHtmlBody(const QString &AHtml);
protected:
	virtual void dropEvent(QDropEvent *AEvent);
	virtual void dragEnterEvent(QDragEnterEvent *AEvent);
	virtual void dragMoveEvent(QDragMoveEvent *AEvent);
	virtual void dragLeaveEvent(QDragLeaveEvent *AEvent);
protected slots:
	void onContentChanged(QWidget *AWidget, const QUuid &AContentId, const QString &AHtml, const IMessageContentOptions &AOptions);
	void onUrlClicked(QWidget *AWidget, const QUrl &AUrl);
	void onCustomContextMenuRequested(const QPoint &APosition);
private:
	Ui::ViewWidgetClass ui;
private:
	IMessageStyle *FMessageStyle;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
private:
	Jid FStreamJid;
	Jid FContactJid;
	QWidget *FStyleWidget;
	QList<IViewDropHandler *> FActiveDropHandlers;
};

#endif // VIEWWIDGET_H
