#include "noticewidget.h"

#include <QHBoxLayout>

NoticeWidget::NoticeWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	ui.wdtActions->setLayout(new QHBoxLayout);
	ui.wdtActions->layout()->setMargin(0);

	FActiveNotice = -1;
	FEmptySince = QDateTime::currentDateTime();

	FUpdateTimer.setInterval(0);
	FUpdateTimer.setSingleShot(true);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateTimerTimeout()));

	connect(ui.cbtClose,SIGNAL(clicked(bool)),SLOT(onCloseButtonClicked(bool)));
}

NoticeWidget::~NoticeWidget()
{

}

QDateTime NoticeWidget::emptySince() const
{
	return FActiveNotice>0 ? QDateTime::currentDateTime() : FEmptySince;
}

int NoticeWidget::activeNotice() const
{
	return FActiveNotice;
}

QList<int> NoticeWidget::noticeQueue() const
{
	return FNoticeQueue.values();
}

IInternalNotice NoticeWidget::noticeById(int ANoticeId) const
{
	return FNotices.value(ANoticeId);
}

int NoticeWidget::insertNotice(const IInternalNotice &ANotice)
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

void NoticeWidget::removeNotice(int ANoticeId)
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
void NoticeWidget::updateNotice()
{
	FUpdateTimer.start();
}

void NoticeWidget::updateWidgets(int ANoticeId)
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
				label->setText(QString("<a href='link'>%1</a>").arg(action->text()));
				connect(label,SIGNAL(linkActivated(const QString &)),action,SLOT(trigger()));
				ui.wdtActions->layout()->addWidget(label);
				FButtonsCleanup.add(label);
			}
			ui.wdtActions->setVisible(!notice.actions.isEmpty());

			setVisible(true);
		}
		else
		{
			setVisible(false);
			FEmptySince = QDateTime::currentDateTime();
		}
		FActiveNotice = ANoticeId;
		emit noticeActivated(ANoticeId);
	}
}

void NoticeWidget::onUpdateTimerTimeout()
{
	updateWidgets(!FNoticeQueue.isEmpty() ? FNoticeQueue.values().first() : -1);
}

void NoticeWidget::onCloseButtonClicked(bool)
{
	removeNotice(FActiveNotice);
}
