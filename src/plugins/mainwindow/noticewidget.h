#ifndef NOTICEWIDGET_H
#define NOTICEWIDGET_H

#include <QTimer>
#include <QWidget>
#include <QObjectCleanupHandler>
#include <definations/resources.h>
#include <definations/stylesheets.h>
#include <interfaces/imainwindow.h>
#include <utils/stylestorage.h>
#include "ui_noticewidget.h"

class NoticeWidget : 
	public QWidget,
	public IInternalNoticeWidget
{
	Q_OBJECT;
	Q_INTERFACES(IInternalNoticeWidget);
public:
	NoticeWidget(QWidget *AParent = NULL);
	~NoticeWidget();
	virtual QWidget *instance() { return this; }
	virtual QDateTime emptySince() const;
	virtual int activeNotice() const;
	virtual QList<int> noticeQueue() const;
	virtual IInternalNotice noticeById(int ANoticeId) const;
	virtual int insertNotice(const IInternalNotice &ANotice);
	virtual void removeNotice(int ANoticeId);
signals:
	void noticeInserted(int ANoticeId);
	void noticeActivated(int ANoticeId);
	void noticeRemoved(int ANoticeId);
protected:
	void updateNotice();
	void updateWidgets(int ANoticeId);
protected slots:
	void onUpdateTimerTimeout();
	void onCloseButtonClicked(bool);
private:
	Ui::NoticeWidgetClass ui;
private:
	int FActiveNotice;
	QTimer FUpdateTimer;
	QDateTime FEmptySince;
	QMultiMap<int, int> FNoticeQueue;
	QMap<int, IInternalNotice> FNotices;
	QObjectCleanupHandler FButtonsCleanup;
};

#endif // NOTICEWIDGET_H
