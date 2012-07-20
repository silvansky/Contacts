#include "selectavatarwidget.h"

SelectAvatarWidget::SelectAvatarWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);

	ui.userPic1->installEventFilter(this);
	ui.userPic2->installEventFilter(this);
	ui.userPic3->installEventFilter(this);
	ui.userPic4->installEventFilter(this);
	ui.userPic5->installEventFilter(this);
	ui.userPic6->installEventFilter(this);
	ui.userPic7->installEventFilter(this);
	ui.userPic8->installEventFilter(this);
	ui.stdPic1->installEventFilter(this);
	ui.stdPic2->installEventFilter(this);
	ui.stdPic3->installEventFilter(this);
	ui.stdPic4->installEventFilter(this);
	ui.stdPic5->installEventFilter(this);
	ui.stdPic6->installEventFilter(this);
	ui.stdPic7->installEventFilter(this);
	ui.noPic->installEventFilter(this);
}

SelectAvatarWidget::~SelectAvatarWidget()
{

}

bool SelectAvatarWidget::eventFilter(QObject *obj, QEvent * event)
{
	if (event->type() == QEvent::MouseButtonPress)
	{
		QLabel * clickedLabel = qobject_cast<QLabel*>(obj);
		if (clickedLabel->pixmap())
		{
			QImage img = clickedLabel->pixmap()->toImage();
			emit avatarSelected(img);
		}
	}
	return QWidget::eventFilter(obj, event);
}
