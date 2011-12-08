#include "noticewidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QDesktopServices>

ChatNoticeWidget::ChatNoticeWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid)
{
	ui.setupUi(this);
	setVisible(false);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_MESSAGEWIDGETS_NOTICEWIDGET);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(ui.cbtClose,STS_MESSAGEWIDGETS_NOTICECLOSEBUTTON);

#ifdef Q_WS_MAC
	ui.wdtButtons->layout()->setSpacing(16);
#endif

	FMessageWidgets = AMessageWidgets;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;

	FActiveNotice = -1;

	FUpdateTimer.setSingleShot(true);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateTimerTimeout()));

	FCloseTimer.setSingleShot(true);
	connect(&FCloseTimer,SIGNAL(timeout()),SLOT(onCloseTimerTimeout()));

	connect(ui.cbtClose,SIGNAL(clicked(bool)),SLOT(onCloseButtonClicked(bool)));
	connect(ui.lblMessage,SIGNAL(linkActivated(const QString &)),SLOT(onMessageLinkActivated(const QString &)));
}

ChatNoticeWidget::~ChatNoticeWidget()
{
	foreach(int noticeId, FNotices.keys()) {
		removeNotice(noticeId); }
}

const Jid &ChatNoticeWidget::streamJid() const
{
	return FStreamJid;
}

void ChatNoticeWidget::setStreamJid(const Jid &AStreamJid)
{
	if (AStreamJid != FStreamJid)
	{
		Jid befour = FStreamJid;
		FStreamJid = AStreamJid;
		emit streamJidChanged(befour);
	}
}

const Jid & ChatNoticeWidget::contactJid() const
{
	return FContactJid;
}

void ChatNoticeWidget::setContactJid(const Jid &AContactJid)
{
	if (AContactJid != FContactJid)
	{
		Jid befour = FContactJid;
		FContactJid = AContactJid;
		emit contactJidChanged(befour);
	}
}

int ChatNoticeWidget::activeNotice() const
{
	return FActiveNotice;
}

QList<int> ChatNoticeWidget::noticeQueue() const
{
	return FNoticeQueue.values();
}

IChatNotice ChatNoticeWidget::noticeById(int ANoticeId) const
{
	return FNotices.value(ANoticeId);
}

int ChatNoticeWidget::insertNotice(const IChatNotice &ANotice)
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

void ChatNoticeWidget::removeNotice(int ANoticeId)
{
	if (FNotices.contains(ANoticeId))
	{
		IChatNotice notice = FNotices.take(ANoticeId);
		FNoticeQueue.remove(notice.priority,ANoticeId);
		qDeleteAll(notice.actions);
		emit noticeRemoved(ANoticeId);
		updateNotice();
	}
}

void ChatNoticeWidget::updateNotice()
{
	FUpdateTimer.start();
}

void ChatNoticeWidget::updateWidgets(int ANoticeId)
{
	if (FActiveNotice != ANoticeId)
	{
		FButtonsCleanup.clear();
		if (ANoticeId > 0)
		{
			const IChatNotice &notice = FNotices.value(ANoticeId);
			if (!notice.iconKey.isEmpty() && !notice.iconStorage.isEmpty())
				IconStorage::staticStorage(notice.iconStorage)->insertAutoIcon(ui.lblIcon,notice.iconKey,0,0,"pixmap");
			else if (!notice.icon.isNull())
				ui.lblIcon->setPixmap(notice.icon.pixmap(notice.icon.availableSizes().value(0)));
			else
				ui.lblIcon->setVisible(false);
			ui.lblMessage->setText(notice.message);

			if (notice.timeout > 0)
				FCloseTimer.start(notice.timeout);
			else
				FCloseTimer.stop();

			foreach(Action *action, notice.actions)
			{
				ActionButton *button = new ActionButton(action, ui.wdtButtons);
				button->addTextFlag(TF_LIGHTSHADOW);
				button->setObjectName(action->property("actionName").toString());
				ui.hltButtonsLayout->insertWidget(ui.hltButtonsLayout->count()-1,button);
				FButtonsCleanup.add(button);
			}

			setVisible(true);
		}
		else
		{
			FCloseTimer.stop();
			setVisible(false);
		}
		FActiveNotice = ANoticeId;
		emit noticeActivated(ANoticeId);
	}
}

void ChatNoticeWidget::paintEvent(QPaintEvent *AEvent)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(AEvent->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatNoticeWidget::onUpdateTimerTimeout()
{
	updateWidgets(!FNoticeQueue.isEmpty() ? FNoticeQueue.values().first() : -1);
}

void ChatNoticeWidget::onCloseTimerTimeout()
{
	if (!underMouse())
		removeNotice(FActiveNotice);
	else
		FCloseTimer.start(500);
}

void ChatNoticeWidget::onCloseButtonClicked(bool)
{
	removeNotice(FActiveNotice);
}

void ChatNoticeWidget::onMessageLinkActivated(const QString &ALink)
{
	QDesktopServices::openUrl(ALink);
}
