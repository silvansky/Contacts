#include "noticewidget.h"

#include <QToolTip>
#include <QToolButton>
#include <QDesktopServices>

NoticeWidget::NoticeWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid)
{
	ui.setupUi(this);
	setVisible(false);
	setPalette(QToolTip::palette());

	FMessageWidgets = AMessageWidgets;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FActiveNotice = -1;

	ui.btbButtons->setCenterButtons(true);
	connect(ui.lblMessage,SIGNAL(linkActivated(const QString &)),SLOT(onMessageLinkActivated(const QString &)));

	FUpdateTimer.setSingleShot(true);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateTimerTimeout()));

	FCloseTimer.setSingleShot(true);
	connect(&FCloseTimer,SIGNAL(timeout()),SLOT(onCloseTimerTimeout()));

	connect(ui.cbtClose,SIGNAL(clicked(bool)),SLOT(onCloseButtonClicked(bool)));
}

NoticeWidget::~NoticeWidget()
{
	foreach(int noticeId, FNotices.keys())
		removeNotice(noticeId);
}

const Jid &NoticeWidget::streamJid() const
{
	return FStreamJid;
}

void NoticeWidget::setStreamJid(const Jid &AStreamJid)
{
	if (AStreamJid != FStreamJid)
	{
		Jid befour = FStreamJid;
		FStreamJid = AStreamJid;
		emit streamJidChanged(befour);
	}
}

const Jid & NoticeWidget::contactJid() const
{
	return FContactJid;
}

void NoticeWidget::setContactJid(const Jid &AContactJid)
{
	if (AContactJid != FContactJid)
	{
		Jid befour = FContactJid;
		FContactJid = AContactJid;
		emit contactJidChanged(befour);
	}
}

int NoticeWidget::activeNotice() const
{
	return FActiveNotice;
}

QList<int> NoticeWidget::noticeQueue() const
{
	return FNoticeQueue.values();
}

INotice NoticeWidget::noticeById(int ANoticeId) const
{
	return FNotices.value(ANoticeId);
}

int NoticeWidget::insertNotice(const INotice &ANotice)
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
		INotice notice = FNotices.take(ANoticeId);
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
		ui.btbButtons->clear();
		if (ANoticeId > 0)
		{
			const INotice &notice = FNotices.value(ANoticeId);
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblIcon,notice.iconKey,0,0,"pixmap");
			ui.lblMessage->setText(notice.message);

			if (notice.timeout > 0)
				FCloseTimer.start(notice.timeout);
			else
				FCloseTimer.stop();

			foreach(Action *action, notice.actions)
			{
				ActionButton *button = new ActionButton(action, ui.btbButtons);
				ui.btbButtons->addButton(button, QDialogButtonBox::ActionRole);
			}

			setVisible(true);
		}
		else
		{
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(ui.lblIcon);
			ui.lblIcon->clear();
			ui.lblMessage->clear();
			FCloseTimer.stop();
			setVisible(false);
		}
		FActiveNotice = ANoticeId;
		emit noticeActivated(ANoticeId);
	}
}

void NoticeWidget::onUpdateTimerTimeout()
{
	updateWidgets(!FNoticeQueue.isEmpty() ? FNoticeQueue.values().first() : -1);
}

void NoticeWidget::onCloseTimerTimeout()
{
	if (!underMouse())
		removeNotice(FActiveNotice);
	else
		FCloseTimer.start(500);
}

void NoticeWidget::onCloseButtonClicked(bool)
{
	removeNotice(FActiveNotice);
}

void NoticeWidget::onMessageLinkActivated(const QString &ALink)
{
	QDesktopServices::openUrl(ALink);
}
