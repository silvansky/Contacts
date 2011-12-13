#include "custominputdialog.h"

#include "customborderstorage.h"
#include <definitions/customborder.h>
#include <definitions/resources.h>
#include "stylestorage.h"
#include <definitions/stylesheets.h>
#include "graphicseffectsstorage.h"
#include <definitions/graphicseffects.h>
#ifdef Q_WS_MAC
# include "macwidgets.h"
#endif

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QKeyEvent>

CustomInputDialog::CustomInputDialog(CustomInputDialog::InputType type, QWidget *AParent) :
	QDialog(AParent),
	inputType(type)
{
	initLayout();

	setAttribute(Qt::WA_DeleteOnClose, false);

	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	setMinimumWidth(250);
	if (border)
	{
		border->setParent(AParent);
		border->setResizable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(border, SIGNAL(closeClicked()), SLOT(reject()));
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}
#ifdef Q_WS_MAC
	setWindowGrowButtonEnabled(this->window(), false);
#endif
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_UTILS_CUSTOMINPUTDIALOG);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);
}

CustomInputDialog::~CustomInputDialog()
{
	if (border)
		border->deleteLater();
}

CustomBorderContainer * CustomInputDialog::windowBorder()
{
	return border;
}

void CustomInputDialog::show()
{
	if (border)
	{
		border->setWindowModality(windowModality());
		// TODO: determine what of these are really needed
		border->layout()->update();
		layout()->update();
		border->adjustSize();
		border->show();
		border->layout()->update();
		border->adjustSize();
	}
	else
		QDialog::show();
}

QString CustomInputDialog::defaultText() const
{
	return valueEdit->text();
}

void CustomInputDialog::setDefaultText(const QString &text)
{
	valueEdit->setText(text);
	valueEdit->selectAll();
}

void CustomInputDialog::setCaptionText(const QString &text)
{
	setWindowTitle(text);
	if (captionLabel)
	{
		captionLabel->setText(text);
		captionLabel->setVisible(!text.isEmpty());
	}
}

void CustomInputDialog::setInfoText(const QString &text)
{
	infoLabel->setText(text);
	infoLabel->setVisible(!text.isEmpty());
}

void CustomInputDialog::setDescriptionText(const QString &text)
{
	descrLabel->setText(text);
	descrLabel->setVisible(!text.isEmpty());
}

void CustomInputDialog::setIcon(const QImage &icon)
{
	iconLabel->setPixmap(QPixmap::fromImage(icon));
	iconLabel->setVisible(!icon.isNull());
}

void CustomInputDialog::setAcceptButtonText(const QString &text)
{
	acceptButton->setText(text);
}

void CustomInputDialog::setRejectButtonText(const QString &text)
{
	rejectButton->setText(text);
}

void CustomInputDialog::setAcceptIsDefault(bool accept)
{
	if (accept)
	{
		acceptButton->setDefault(true);
		rejectButton->setDefault(false);
	}
	else
	{
		acceptButton->setDefault(false);
		rejectButton->setDefault(true);
	}
}

bool CustomInputDialog::deleteOnClose() const
{
	return (border ? (QWidget*)border : (QWidget*)this)->testAttribute(Qt::WA_DeleteOnClose);
}

void CustomInputDialog::setDeleteOnClose(bool on)
{
	(border ? (QWidget*)border : (QWidget*)this)->setAttribute(Qt::WA_DeleteOnClose, on);
}

void CustomInputDialog::onAcceptButtonClicked()
{
	if (inputType == String)
	{
		emit stringAccepted(valueEdit->text());
		if (border)
			border->close();
		else
			close();
	}
	else
		accept();
}

void CustomInputDialog::onRejectButtonClicked()
{
	reject();
}

void CustomInputDialog::onTextChanged(const QString & text)
{
	acceptButton->setEnabled(!text.isEmpty());
}

void CustomInputDialog::initLayout()
{
	QHBoxLayout * myLayout = new QHBoxLayout(this);
	QWidget * container = new QWidget(this);
	container->setObjectName("containerWidget");
	QVBoxLayout * mainLayout = new QVBoxLayout(container);
	QHBoxLayout * captionLayout = new QHBoxLayout;
	captionLayout->addWidget(iconLabel = new CustomLabel);
	iconLabel->setMinimumSize(0, 0);
	iconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	iconLabel->setVisible(false);
#ifdef Q_WS_MAC
	captionLayout->addWidget(infoLabel = new CustomLabel);
	mainLayout->addLayout(captionLayout);
	captionLabel = NULL;
#else
	captionLayout->addWidget(captionLabel = new CustomLabel);
	captionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	mainLayout->addLayout(captionLayout);
	mainLayout->addWidget(infoLabel = new CustomLabel);
#endif
	infoLabel->setVisible(false);
	mainLayout->addWidget(valueEdit = new QLineEdit);
	connect(valueEdit, SIGNAL(textChanged(const QString &)), SLOT(onTextChanged(const QString &)));
	mainLayout->addWidget(descrLabel = new CustomLabel);
	descrLabel->setVisible(false);
	QHBoxLayout * buttonsLayout = new QHBoxLayout;
	buttonsLayout->addStretch();
	acceptButton = new QPushButton;
	acceptButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	rejectButton = new QPushButton;
#ifdef Q_WS_MAC
	buttonsLayout->addWidget(rejectButton);
	buttonsLayout->addWidget(acceptButton);
#else
	buttonsLayout->addWidget(acceptButton);
	buttonsLayout->addWidget(rejectButton);
#endif
	buttonsLayout->setContentsMargins(0, 5, 0, 0);
	mainLayout->addLayout(buttonsLayout);
	container->setLayout(mainLayout);
	myLayout->setContentsMargins(0, 0, 0, 0);
	myLayout->setSpacing(0);
	myLayout->addWidget(container);
	setLayout(myLayout);
	iconLabel->setObjectName("iconLabel");
	if (captionLabel)
		captionLabel->setObjectName("captionLabel");
	infoLabel->setObjectName("infoLabel");
	descrLabel->setObjectName("descrLabel");
	valueEdit->setObjectName("valueEdit");
	acceptButton->setObjectName("acceptButton");
	rejectButton->setObjectName("rejectButton");
	infoLabel->setWordWrap(true);
	descrLabel->setWordWrap(true);
	valueEdit->selectAll();
	valueEdit->setVisible(inputType == String);
	connect(infoLabel, SIGNAL(linkActivated(QString)), SIGNAL(linkActivated(QString)));
	connect(descrLabel, SIGNAL(linkActivated(QString)), SIGNAL(linkActivated(QString)));
	connect(acceptButton, SIGNAL(clicked()), SLOT(onAcceptButtonClicked()));
	connect(rejectButton, SIGNAL(clicked()), SLOT(onRejectButtonClicked()));
	acceptButton->setDefault(true);
	acceptButton->setAutoDefault(false);
	rejectButton->setAutoDefault(false);
	container->installEventFilter(this);
	valueEdit->installEventFilter(this);
	// default strings
	setCaptionText(inputType == String ? tr("Enter string value") : tr("Yes or no?"));
	setAcceptButtonText(inputType == String ? tr("OK") : tr("Yes"));
	setRejectButtonText(inputType == String ? tr("Cancel") : tr("No"));
	rejectButton->setVisible(inputType != Info);
	acceptButton->setEnabled(inputType != String);
}
