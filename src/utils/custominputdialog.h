#ifndef CUSTOMINPUTDIALOG_H
#define CUSTOMINPUTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "utilsexport.h"
#include "custombordercontainer.h"
#include "customlabel.h"

class UTILS_EXPORT CustomInputDialog : public QDialog
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
	CustomInputDialog(InputType inputType, QWidget *AParent = NULL);
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
	bool deleteOnClose() const;
	void setDeleteOnClose(bool on);
signals:
	void stringAccepted(const QString & value);
	void linkActivated(const QString &link);
protected slots:
	void onAcceptButtonClicked();
	void onRejectButtonClicked();
	void onTextChanged(const QString &);
private:
	void initLayout();
protected:
	bool eventFilter(QObject * obj, QEvent * evt);
private:
	CustomBorderContainer * border;
	InputType inputType;
	QLineEdit * valueEdit;
	CustomLabel * captionLabel;
	CustomLabel * infoLabel;
	CustomLabel * iconLabel;
	CustomLabel * descrLabel;
	QPushButton * acceptButton;
	QPushButton * rejectButton;
};

#endif // CUSTOMINPUTDIALOG_H
