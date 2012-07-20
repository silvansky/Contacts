#include "aboutqtdialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QStyleOption>
#include <QKeyEvent>
#include <QPushButton>
#include <utils/customborderstorage.h>
#include <utils/stylestorage.h>
#include <utils/graphicseffectsstorage.h>
#include <utils/widgetmanager.h>
#include <utils/customlabel.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/customborder.h>
#include <definitions/graphicseffects.h>
#include <definitions/stylesheets.h>
#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
#endif

AboutQtDialog::AboutQtDialog() : QWidget(NULL)
{
	setWindowTitle(tr("About Qt"));
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_PLUGINMANAGER_ABOUT_QT);

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	if (border)
	{
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		border->setResizable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose, true);
		setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
	}

#ifdef Q_WS_MAC
	setWindowGrowButtonEnabled(this->window(), false);
#endif

	// creating layouts and items
	// main layout
	setLayout(new QVBoxLayout(this));

	layout()->setContentsMargins(8, 18, 8, 6);
	layout()->setSpacing(14);

	// text and icon layout
	QHBoxLayout * textLayout = new QHBoxLayout;
	textLayout->setSpacing(6);
	CustomLabel * icon = new CustomLabel(this);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(icon, MNI_PLUGINMANAGER_ABOUT_QT, 0, 0, "pixmap");
	icon->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	icon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
	textLayout->addWidget(icon);
	QVBoxLayout * captLayout = new QVBoxLayout;
	captLayout->setSpacing(8);
	CustomLabel * caption = new CustomLabel(this);
	caption->setObjectName("caption");
	caption->setText(tr("About Qt"));

	CustomLabel * text = new CustomLabel(this);
	text->setTextFormat(Qt::RichText);
	text->setWordWrap(true);
	text->setMaximumWidth(350);
	text->setOpenExternalLinks(true);
	text->setObjectName("lblAboutQtText");

	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(text, GFX_LABELS);
	QString localizedText = tr("<p>This program uses Qt version %1.</p>"
				   "<p>Qt is a C++ toolkit for cross-platform application "
				   "development.</p>"
				   "<p>Qt provides single-source portability across MS&nbsp;Windows, "
				   "Mac&nbsp;OS&nbsp;X, Linux, and all major commercial Unix variants. "
				   "Qt is also available for embedded devices as Qt for Embedded Linux "
				   "and Qt for Windows CE.</p>"
				   "<p>Qt licensed under the GNU LGPL version 2.1 is appropriate for the "
				   "development of Qt applications (proprietary or open source) provided "
				   "you can comply with the terms and conditions of the GNU LGPL version "
				   "2.1.</p>"
				   "<p>Please see <a href=\"http://qt.nokia.com/products/licensing\">qt.nokia.com/products/licensing</a> "
				   "for an overview of Qt licensing.</p>"
				   "<p>Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).</p>"
				   "<p>Qt is a Nokia product. See <a href=\"http://qt.nokia.com/\">qt.nokia.com</a> "
				   "for more information.</p>").arg(QLatin1String(QT_VERSION_STR));
	// adding shadow (it's a rich text)
	text->setText(localizedText);
	captLayout->addWidget(caption);
	captLayout->addWidget(text);
	textLayout->addItem(captLayout);

	// button layout
	QHBoxLayout * buttonLayout = new QHBoxLayout;
	buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
	QPushButton * okButton = new QPushButton(this);
	okButton->setDefault(true);
	okButton->setAutoDefault(false);
	okButton->setText(tr("OK"));
	connect(okButton, SIGNAL(clicked()), (parentWidget() ? parentWidget() : this), SLOT(close()));
	buttonLayout->addWidget(okButton);

	layout()->addItem(textLayout);
	layout()->addItem(buttonLayout);
}

void AboutQtDialog::paintEvent(QPaintEvent * pe)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(pe->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void AboutQtDialog::keyPressEvent(QKeyEvent * ke)
{
	if (ke->key() == Qt::Key_Escape)
		(parentWidget() ? parentWidget() : (QWidget*)this)->close();
}

void AboutQtDialog::aboutQt()
{
	AboutQtDialog * dialog = new AboutQtDialog;
	WidgetManager::showActivateRaiseWindow(dialog->window());
	dialog->window()->adjustSize();
	WidgetManager::alignWindow(dialog->window(), Qt::AlignCenter);
}
