#include "noticewidget.h"

#include <QHBoxLayout>

InternalNoticeWidget::InternalNoticeWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_MAINWINDOW_NOTICEWIDGET);

	ui.wdtActions->setLayout(new QHBoxLayout);
	ui.wdtActions->layout()->setMargin(0);

	FActiveNotice = -1;

	FReadyTimer.setInterval(10*1000);
	FReadyTimer.setSingleShot(false);
	connect(&FReadyTimer,SIGNAL(timeout()),SLOT(onReadyTimerTimeout()));
	FReadyTimer.start();

	FUpdateTimer.setInterval(0);
	FUpdateTimer.setSingleShot(true);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateTimerTimeout()));

	connect(ui.cbtClose,SIGNAL(clicked(bool)),SLOT(onCloseButtonClicked(bool)));
}

InternalNoticeWidget::~InternalNoticeWidget()
{

}

bool InternalNoticeWidget::isEmpty() const
{
	return FNotices.isEmpty();
}

int InternalNoticeWidget::activeNotice() const
{
	return FActiveNotice;
}

QList<int> InternalNoticeWidget::noticeQueue() const
{
	return FNoticeQueue.values();
}

IInternalNotice InternalNoticeWidget::noticeById(int ANoticeId) const
{
	return FNotices.value(ANoticeId);
}

int InternalNoticeWidget::insertNotice(const IInternalNotice &ANotice)
{
	int noticeId = -1;
	if (ANotice.priority>0)
	{
		while (noticeId<=0 || FNotices.contains(noticeId))
			noticeId = qrand();

		FNotices.insert(noticeId,ANotice);
		FNoticeQueue.insertMulti(ANotice.priority,noticeId);
		emit noticeInserted(noticeId);
		updateNotice();
	}
	return noticeId;
}

void InternalNoticeWidget::removeNotice(int ANoticeId)
{
	if (FNotices.contains(ANoticeId))
	{
		IInternalNotice notice = FNotices.take(ANoticeId);
		FNoticeQueue.remove(notice.priority,ANoticeId);
		qDeleteAll(notice.actions);
		emit noticeRemoved(ANoticeId);
		updateNotice();
	}
}
void InternalNoticeWidget::updateNotice()
{
	FUpdateTimer.start();
}

void InternalNoticeWidget::updateWidgets(int ANoticeId)
{
	if (FActiveNotice != ANoticeId)
	{
		FButtonsCleanup.clear();
		if (ANoticeId > 0)
		{
			const IInternalNotice &notice = FNotices.value(ANoticeId);
			if (!notice.iconKey.isEmpty() && !notice.iconStorage.isEmpty())
				IconStorage::staticStorage(notice.iconStorage)->insertAutoIcon(ui.lblIcon,notice.iconKey,0,0,"pixmap");
			else if (!notice.icon.isNull())
				ui.lblIcon->setPixmap(notice.icon.pixmap(notice.icon.availableSizes().value(0)));
			else
				ui.lblIcon->setVisible(false);

			ui.lblCaption->setText(notice.caption);
			ui.lblMessage->setText(notice.message);

			foreach(Action *action, notice.actions)
			{
				QLabel *label = new QLabel(ui.wdtActions);
				label->setTextFormat(Qt::RichText);
				label->setWordWrap(true);
				label->setText(QString("<a href='link'>%1</a>").arg(action->text()));
				connect(label,SIGNAL(linkActivated(const QString &)),action,SLOT(trigger()));
				connect(action,SIGNAL(triggered()),SLOT(onNoticeActionTriggered()));
				ui.wdtActions->layout()->addWidget(label);
				FButtonsCleanup.add(label);
			}
			ui.wdtActions->setVisible(!notice.actions.isEmpty());

			setVisible(true);
			FReadyTimer.stop();
		}
		else
		{
			setVisible(false);
			FReadyTimer.start();
		}
		FActiveNotice = ANoticeId;
		emit noticeActivated(ANoticeId);
	}
}

void InternalNoticeWidget::onReadyTimerTimeout()
{
	emit noticeWidgetReady();
}

void InternalNoticeWidget::onUpdateTimerTimeout()
{
	updateWidgets(!FNoticeQueue.isEmpty() ? FNoticeQueue.values().first() : -1);
}

void InternalNoticeWidget::onNoticeActionTriggered()
{
	removeNotice(FActiveNotice);
}

void InternalNoticeWidget::onCloseButtonClicked(bool)
{
	removeNotice(FActiveNotice);
}
