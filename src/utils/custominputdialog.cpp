#include "custominputdialog.h"

#include "customborderstorage.h"
#include <definitions/customborder.h>
#include <definitions/resources.h>
#include "stylestorage.h"
#include <definitions/stylesheets.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QKeyEvent>

CustomInputDialog::CustomInputDialog(CustomInputDialog::InputType type) :
	QWidget(0),
	inputType(type)
{
	initLayout();

	setAttribute(Qt::WA_DeleteOnClose, false);

	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_DIALOG);
	setMinimumWidth(250);
	if (border)
	{
		border->setResizable(false);
		border->setMinimizeButtonVisible(false);
		border->setMaximizeButtonVisible(false);
		connect(this, SIGNAL(accepted()), border, SLOT(close()));
		connect(this, SIGNAL(rejected()), border, SLOT(close()));
		connect(border, SIGNAL(closeClicked()), SIGNAL(rejected()));
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_UTILS_CUSTOMINPUTDIALOG);
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
		// TODO: determine what of these are really needed
		border->layout()->update();
		layout()->update();
		border->adjustSize();
		border->show();
		border->layout()->update();
		border->adjustSize();
	}
	else
		QWidget::show();
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
	captionLabel->setText(text);
	setWindowTitle(text);
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

void CustomInputDialog::onAcceptButtonClicked()
{
	if (inputType == String)
		emit stringAccepted(valueEdit->text());
	else
		emit accepted();
	if (border)
		border->close();
	else
		close();
}

void CustomInputDialog::onRejectButtonClicked()
{
	emit rejected();
	if (border)
		border->close();
	else
		close();
}

void CustomInputDialog::initLayout()
{
	QHBoxLayout * myLayout = new QHBoxLayout(this);
	QWidget * container = new QWidget(this);
	container->setObjectName("containerWidget");
	QVBoxLayout * mainLayout = new QVBoxLayout(container);
	QHBoxLayout * captionLayout = new QHBoxLayout;
	captionLayout->addWidget(iconLabel = new QLabel);
	iconLabel->setMinimumSize(0, 0);
	iconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	iconLabel->setVisible(false);
	captionLayout->addWidget(captionLabel = new QLabel);
	captionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	mainLayout->addLayout(captionLayout);
	mainLayout->addWidget(infoLabel = new QLabel);
	infoLabel->setVisible(false);
	mainLayout->addWidget(valueEdit = new QLineEdit);
	mainLayout->addWidget(descrLabel = new QLabel);
	descrLabel->setVisible(false);
	QHBoxLayout * buttonsLayout = new QHBoxLayout;
	buttonsLayout->addStretch();
	buttonsLayout->addWidget(acceptButton = new QPushButton);
	buttonsLayout->addWidget(rejectButton = new QPushButton);
	mainLayout->addLayout(buttonsLayout);
	container->setLayout(mainLayout);
	myLayout->setContentsMargins(0, 0, 0, 0);
	myLayout->setSpacing(0);
	myLayout->addWidget(container);
	setLayout(myLayout);
	iconLabel->setObjectName("iconLabel");
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
	captionLabel->setText(inputType == String ? tr("Enter string value") : tr("Yes or no?"));
	acceptButton->setText(inputType == String ? tr("OK") : tr("Yes"));
	rejectButton->setText(inputType == String ? tr("Cancel") : tr("No"));
}

bool CustomInputDialog::eventFilter(QObject * obj, QEvent * evt)
{
	switch(evt->type())
	{
	case QEvent::KeyPress:
	{
		QKeyEvent * keyEvent = (QKeyEvent*)evt;
		switch(keyEvent->key())
		{
		case Qt::Key_Return:
		case Qt::Key_Enter:
			if (acceptButton->isDefault())
				onAcceptButtonClicked();
			else
				onRejectButtonClicked();
			break;
		case Qt::Key_Escape:
			onRejectButtonClicked();
			break;
		}
	}
		break;
	default:
		break;
	}
	return QWidget::eventFilter(obj, evt);
}
