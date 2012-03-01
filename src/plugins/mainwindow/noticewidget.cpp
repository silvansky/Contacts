#include "noticewidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QImage>
#include <QHBoxLayout>
#include <utils/actionbutton.h>
#include <utils/graphicseffectsstorage.h>
#include <utils/iconstorage.h>
#include <definitions/resources.h>
#include <definitions/graphicseffects.h>
#include <definitions/textflags.h>
#include <definitions/menuicons.h>

InternalNoticeWidget::InternalNoticeWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_MAINWINDOW_NOTICEWIDGET);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(ui.cbtClose,STS_MESSAGEWIDGETS_NOTICECLOSEBUTTON);

	ui.wdtActions->setLayout(new QHBoxLayout);
	ui.wdtActions->layout()->setMargin(0);

	FActiveNotice = -1;

#ifdef DEBUG_ENABLED
	// 5 sec in debug
	FReadyTimer.setInterval(5*1000);
#else
	// 1 hour in release
	FReadyTimer.setInterval(60*60*1000);
#endif
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
		FActionLabels.clear();
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
		FActionWidgetsCleanup.clear();
		static QSpacerItem * spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding);
		ui.wdtActions->layout()->removeItem(spacer);
		if (ANoticeId > 0)
		{
			const IInternalNotice &notice = FNotices.value(ANoticeId);

			ui.lblIcon->setVisible(true);
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
				IInternalNotice::ActionType type = (IInternalNotice::ActionType)action->data(IInternalNotice::TypeRole).toInt();
				switch (type)
				{
				case IInternalNotice::ImageAction:
				{
					QImage img = action->data(IInternalNotice::ImageRole).value<QImage>();
					if (!img.isNull())
					{
						QLabel * imageLabel = new QLabel(ui.wdtActions);
						imageLabel->setPixmap(QPixmap::fromImage(img));
						imageLabel->setCursor(QCursor(Qt::PointingHandCursor));
						imageLabel->setToolTip(action->text());
						imageLabel->setProperty("ignoreFilter", true);
						FActionLabels.insert(imageLabel, action);
						imageLabel->installEventFilter(this);
						ui.wdtActions->layout()->addWidget(imageLabel);
						FActionWidgetsCleanup.add(imageLabel);
					}
					break;
				}
				case IInternalNotice::LinkAction:
				{
					// TODO: create a label with a link
					break;
				}
				case IInternalNotice::ButtonAction:
				default:
				{
					ActionButton *button = new ActionButton(action, ui.wdtActions);
					button->addTextFlag(TF_LIGHTSHADOW);
					button->setText(action->text());
					connect(action,SIGNAL(triggered()),SLOT(onNoticeActionTriggered()));
					ui.wdtActions->layout()->addWidget(button);
					FActionWidgetsCleanup.add(button);
					break;
				}
				}
			}
			ui.wdtActions->layout()->addItem(spacer);
			ui.wdtActions->setVisible(!notice.actions.isEmpty());

			setVisible(true);
			FReadyTimer.stop();

			LogDetail(QString("[InternalNoticeWidget] Internal notice activated '%1'").arg(notice.caption));
		}
		else
		{
			setVisible(false);
			FReadyTimer.start();

			LogDetail(QString("[InternalNoticeWidget] Internal notice widget closed"));
		}
		FActiveNotice = ANoticeId;
		emit noticeActivated(ANoticeId);
	}
}

void InternalNoticeWidget::paintEvent(QPaintEvent *AEvent)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(AEvent->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

bool InternalNoticeWidget::eventFilter(QObject * obj, QEvent * evt)
{
	if (evt->type() == QEvent::MouseButtonRelease)
	{
		if (QLabel * lbl = qobject_cast<QLabel*>(obj))
		{
			Action * a = FActionLabels.value(lbl);
			a->trigger();
		}
	}
	return QWidget::eventFilter(obj, evt);
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
