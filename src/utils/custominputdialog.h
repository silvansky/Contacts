#ifndef CUSTOMINPUTDIALOG_H
#define CUSTOMINPUTDIALOG_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "utilsexport.h"
#include "custombordercontainer.h"

class UTILS_EXPORT CustomInputDialog : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString defaultText READ defaultText WRITE setDefaultText)
public:
	enum InputType
	{
		String,
		Info,
		None
	};
	CustomInputDialog(InputType inputType);
	~CustomInputDialog();
	CustomBorderContainer * windowBorder();
	void show();
	QString defaultText() const;
	void setDefaultText(const QString &text);
	void setCaptionText(const QString &text);
	void setInfoText(const QString &text);
	void setDescriptionText(const QString &text);
	void setIcon(const QImage &icon);
	void setAcceptButtonText(const QString &text);
	void setRejectButtonText(const QString &text);
	void setAcceptIsDefault(bool);
signals:
	void accepted();
	void stringAccepted(const QString & value);
	void rejected();
	void linkActivated(const QString &link);
protected slots:
	void onAcceptButtonClicked();
	void onRejectButtonClicked();
	void onTextChanged(const QString &);
private:
	void initLayout();
protected:
	bool eventFilter(QObject *, QEvent *);
private:
	CustomBorderContainer * border;
	InputType inputType;
	QLineEdit * valueEdit;
	QLabel * captionLabel;
	QLabel * infoLabel;
	QLabel * iconLabel;
	QLabel * descrLabel;
	QPushButton * acceptButton;
	QPushButton * rejectButton;
};

#endif // CUSTOMINPUTDIALOG_H
