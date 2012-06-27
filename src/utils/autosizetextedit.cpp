#include "autosizetextedit.h"

#include <QFrame>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include "stylestorage.h"

AutoSizeTextEdit::AutoSizeTextEdit(QWidget *AParent) : QTextBrowser(AParent)
{
	FAutoResize = true;
	FMinimumLines = 1;
	FMaximumLines = 0;
	
	setOpenLinks(false);
	setOpenExternalLinks(false);
	setAttribute(Qt::WA_MacShowFocusRect, false);
	setTextInteractionFlags(Qt::TextEditorInteraction);
	document()->setDocumentMargin(7);
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);

	connect(verticalScrollBar(), SIGNAL(rangeChanged(int,int)), SLOT(onScrollBarRangeChanged(int,int)));
	connect(this,SIGNAL(textChanged()),SLOT(onTextChanged()));
}

AutoSizeTextEdit::~AutoSizeTextEdit()
{

}

bool AutoSizeTextEdit::autoResize() const
{
	return FAutoResize;
}

void AutoSizeTextEdit::setAutoResize(bool AResize)
{
	if (AResize != FAutoResize)
	{
		FAutoResize = AResize;
		updateGeometry();
	}
}

int AutoSizeTextEdit::minimumLines() const
{
	return FMinimumLines;
}

void AutoSizeTextEdit::setMinimumLines(int ALines)
{
	if (ALines != FMinimumLines)
	{
		FMinimumLines = ALines>0 ? ALines : 1;
		setMinimumHeight(textHeight(FMinimumLines));
		updateGeometry();
	}
}

int AutoSizeTextEdit::maximumLines() const
{
	return FMaximumLines;
}

void AutoSizeTextEdit::setMaximumLines(int ALines)
{
	if (ALines != FMaximumLines)
	{
		if (ALines > 0)
		{
			FMaximumLines = ALines;
			setMaximumHeight(textHeight(FMaximumLines));
		}
		else
		{
			FMaximumLines = 0;
			setMaximumHeight(QWIDGETSIZE_MAX);
		}
		updateGeometry();
	}
}

QSize AutoSizeTextEdit::sizeHint() const
{
	QSize sh = QTextEdit::sizeHint();
	sh.setHeight(qMin(textHeight(!FAutoResize ? FMinimumLines : 0),maximumHeight()));
	return sh;
}

QSize AutoSizeTextEdit::minimumSizeHint() const
{
	QSize sh = QTextEdit::minimumSizeHint();
	sh.setHeight(textHeight(FMinimumLines));
	return sh;
}

void AutoSizeTextEdit::keyPressEvent(QKeyEvent *ev)
{
	// ignoring QTextBrowser's implementation
	QTextEdit::keyPressEvent(ev);
}

int AutoSizeTextEdit::textHeight(int ALines) const
{
	if (ALines > 0)
		return fontMetrics().height()*ALines + (frameWidth() + qRound(document()->documentMargin()))*2;
	else
		return qRound(document()->documentLayout()->documentSize().height()) + frameWidth()*2;
}

void AutoSizeTextEdit::onTextChanged()
{
	updateGeometry();
}

void AutoSizeTextEdit::onScrollBarRangeChanged(int min, int max)
{
	Q_UNUSED(min)
	Q_UNUSED(max)
	StyleStorage::updateStyle(this);
}
