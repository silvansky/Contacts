#ifndef NOTICEWIDGET_H
#define NOTICEWIDGET_H

#include <QTimer>
#include <QWidget>
#include <QObjectCleanupHandler>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imainwindow.h>
#include <utils/stylestorage.h>
#include "ui_noticewidget.h"

class InternalNoticeWidget :
	public QWidget,
	public IInternalNoticeWidget
{
	Q_OBJECT
	Q_INTERFACES(IInternalNoticeWidget)
public:
	InternalNoticeWidget(QWidget *AParent = NULL);
	~InternalNoticeWidget();
	virtual QWidget *instance() { return this; }
	virtual bool isEmpty() const;
	virtual int activeNotice() const;
	virtual QList<int> noticeQueue() const;
	virtual IInternalNotice noticeById(int ANoticeId) const;
	virtual int insertNotice(const IInternalNotice &ANotice);
	virtual void removeNotice(int ANoticeId);
signals:
	void noticeWidgetReady();
	void noticeInserted(int ANoticeId);
	void noticeActivated(int ANoticeId);
	void noticeRemoved(int ANoticeId);
protected:
	void updateNotice();
	void updateWidgets(int ANoticeId);
protected slots:
	void onReadyTimerTimeout();
	void onUpdateTimerTimeout();
	void onNoticeActionTriggered();
	void onCloseButtonClicked(bool);
protected:
	bool eventFilter(QObject *, QEvent *);
	void paintEvent(QPaintEvent *);
private:
	Ui::NoticeWidgetClass ui;
private:
	int FActiveNotice;
	QTimer FReadyTimer;
	QTimer FUpdateTimer;
	QMultiMap<int, int> FNoticeQueue;
	QMap<int, IInternalNotice> FNotices;
	QObjectCleanupHandler FButtonsCleanup;
};

#endif // NOTICEWIDGET_H
