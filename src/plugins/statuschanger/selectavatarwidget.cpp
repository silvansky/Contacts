#include "selectavatarwidget.h"
#include "ui_selectavatarwidget.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

SelectAvatarWidget::SelectAvatarWidget(QWidget *parent) :
		QWidget(parent),
		ui(new Ui::SelectAvatarWidget)
{
	ui->setupUi(this);
	ui->userPic1->installEventFilter(this);
	ui->userPic2->installEventFilter(this);
	ui->userPic3->installEventFilter(this);
	ui->userPic4->installEventFilter(this);
	ui->userPic5->installEventFilter(this);
	ui->userPic6->installEventFilter(this);
	ui->userPic7->installEventFilter(this);
	ui->userPic8->installEventFilter(this);
	ui->stdPic1->installEventFilter(this);
	ui->stdPic2->installEventFilter(this);
	ui->stdPic3->installEventFilter(this);
	ui->stdPic4->installEventFilter(this);
	ui->stdPic5->installEventFilter(this);
	ui->stdPic6->installEventFilter(this);
	ui->stdPic7->installEventFilter(this);
	ui->noPic->installEventFilter(this);
}

SelectAvatarWidget::~SelectAvatarWidget()
{
	delete ui;
}

void SelectAvatarWidget::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
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
//		hide();
	}
	return QWidget::eventFilter(obj, event);
}

/*void SelectAvatarWidget::on_uploadButton_clicked()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open image"), "", tr("Image Files (*.png *.jpg *.bmp *.gif)"));
	if (!filename.isEmpty())
	{
		QImage image(filename);
		if (!image.isNull())
		{
			emit avatarSelected(image);
			hide();
		}
	}
}

void SelectAvatarWidget::on_profileButton_clicked()
{
	QDesktopServices::openUrl(QUrl("id.rambler.ru"));
}
*/
