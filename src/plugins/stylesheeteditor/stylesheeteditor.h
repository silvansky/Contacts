/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef STYLESHEETEDITOR_H
#define STYLESHEETEDITOR_H

#include <QTextEdit>
#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include "qtgradientmanager.h"
#include "teststylesform.h"

#include <QDialogButtonBox>

#include <utils/stylestorage.h>
#include <utils/customborderstorage.h>

class StyleSheetEditor : public QTextEdit
{
	Q_OBJECT
public:
	StyleSheetEditor(QWidget *parent = 0);
};

// Edit a style sheet.
class StyleSheetEditorDialog : public QDialog
{
	Q_OBJECT
public:
	StyleSheetEditorDialog(QWidget *parent);
	~StyleSheetEditorDialog();
	QString text() const;
	void setText(const QString &t);

	static bool isStyleSheetValid(const QString &styleSheet);


private slots:
	void validateStyleSheet();
	void slotContextMenuRequested(const QPoint &pos);
	void slotAddGradient(const QString &property);
	void slotAddColor(const QString &property);
	void slotAddFont();
	void slotClicked(QAbstractButton*);
	void onStorageChanged();
	void onComboBoxSelectionChanged(const QString& key);
signals:
	void styleSheetChanged(const QString&);
	void fileOpened(const QString&);
	void resetStyleSheet();

protected:
	QDialogButtonBox *buttonBox() const;
	void setOkButtonEnabled(bool v);
	void loadFile(const QString & fileName);
	void saveFile();

private:
	void insertCssProperty(const QString &name, const QString &value);

	QDialogButtonBox *m_buttonBox;
	StyleSheetEditor *m_editor;
	QLabel *m_validityLabel;
	QAction *m_addGradientAction;
	QAction *m_addColorAction;
	QAction *m_addFontAction;
	QtGradientManager * gm;
	QPushButton * pbOpen, * pbClose, * pbPreview, * pbSave, * pbReset, * pbTest;
	QString activeFileName;
	bool fileSaved;
	StyleStorage * styleStorage;
	QComboBox * styleKeys;
	QString lastKey;
	TestStylesForm * testForm;
	CustomBorderContainer * testFormContainer;
};


#endif // STYLESHEETEDITOR_H
