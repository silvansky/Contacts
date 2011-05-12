#include "actionbutton.h"

#include <QStylePainter>
#include <QStyleOptionButton>

ActionButton::ActionButton(QWidget *AParent) : QPushButton(AParent)
{
	FAction = NULL;
	additionalTextFlag = 0;
}

ActionButton::ActionButton(Action *AAction, QWidget *AParent) : QPushButton(AParent)
{
	FAction = NULL;
	additionalTextFlag = 0;
	setAction(AAction);
}

Action *ActionButton::action() const
{
	return FAction;
}

void ActionButton::setAction(Action *AAction)
{
	if (FAction != AAction)
	{
		if (FAction)
		{
			disconnect(FAction,0,this,0);
		}

		FAction = AAction;
		onActionChanged();

		if (FAction)
		{
			connect(this,SIGNAL(clicked()),FAction,SLOT(trigger()));
			connect(FAction,SIGNAL(changed()),SLOT(onActionChanged()));
			connect(FAction,SIGNAL(actionDestroyed(Action *)),SLOT(onActionDestroyed(Action *)));
			setActionString(AAction->data(Action::DR_UserDefined  + 1).toString());
		}

		emit actionChanged();
	}
}

QString ActionButton::actionString()
{
	return property("actionString").toString();
}

void ActionButton::setActionString(const QString& s)
{
	setProperty("actionString", s);
	setStyleSheet(styleSheet());
}

void ActionButton::addTextFlag(int flag)
{
	additionalTextFlag = flag;
}

void ActionButton::onActionChanged()
{
	if (FAction)
	{
		setIcon(FAction->icon());
		setText(FAction->text());
		setMenu(FAction->menu());
	}
	else
	{
		setIcon(QIcon());
		setText(QString::null);
		setMenu(NULL);
	}
	emit buttonChanged();
}

void ActionButton::onActionDestroyed(Action *AAction)
{
	if (FAction == AAction)
	{
		setAction(NULL);
	}
}

void ActionButton::paintEvent(QPaintEvent *)
{
	QStylePainter p(this);
	QStyleOptionButton option;
	initStyleOption(&option);
	//option.text = QString::null;
	p.drawControl(QStyle::CE_PushButtonBevel, option);
	//p.drawControl(QStyle::CE_PushButtonLabel, option);
	p.drawItemText(option.rect, Qt::AlignCenter | additionalTextFlag, palette(), isEnabled(), text(), QPalette::ButtonText);
}
