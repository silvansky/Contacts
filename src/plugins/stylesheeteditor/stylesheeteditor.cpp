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

#include "stylesheeteditor.h"
#include "csshighlighter.h"
#include "qtgradientviewdialog.h"
#include "qtgradientutils.h"
#include "qdesigner_utils.h"

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QExtensionManager>

#include <QSignalMapper>
#include <QAction>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFontDialog>
#include <QMenu>
#include <QPushButton>
#include <QTextDocument>
#include <QToolBar>
#include <QVBoxLayout>
#include <QListView>
#include "private/qcssparser_p.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QByteArray>

#include <QDebug>

#include <definitions/resources.h>
#include <definitions/customborder.h>

StyleSheetEditor::StyleSheetEditor(QWidget *parent)
	: QTextEdit(parent)
{
	setFontFamily("Courier");
	setFontPointSize(12);
	setTabStopWidth(fontMetrics().width(QLatin1Char(' ')) * 14);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	new CssHighlighter(document());
}

// --- StyleSheetEditorDialog
StyleSheetEditorDialog::StyleSheetEditorDialog(QWidget *parent):
	QDialog(parent),
	m_buttonBox(new QDialogButtonBox()),
	m_editor(new StyleSheetEditor),
	m_validityLabel(new QLabel(tr("Valid Style Sheet"))),
	m_addGradientAction(new QAction(tr("Add Gradient..."), this)),
	m_addColorAction(new QAction(tr("Add Color..."), this)),
	m_addFontAction(new QAction(tr("Add Font..."), this))
{

	m_buttonBox->setObjectName("buttonBox");
	// buttons
	pbOpen = new QPushButton(tr("Open"));
	pbClose = new QPushButton(tr("Close"));
	pbPreview = new QPushButton(tr("Preview"));
	pbSave = new QPushButton(tr("Save"));
	pbReset = new QPushButton(tr("Reset"));
	pbTest = new QPushButton(tr("Test Form"));

	testForm = new TestStylesForm;
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(testForm, "stylesheeteditorTestForm");
	setStyleSheet("QToolBar { border: none; background: transparent; }");
	testFormContainer = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(testForm, CBS_WINDOW);
	//	testForm->setObjectName("testForm");
	//	testForm->setStyleSheet("#testForm { background-color: rgba(65, 70, 77, 245); } QGroupBox { background: transparent; }");

	setStyleSheet("StyleSheetEditor { border: 1px solid black; background-color: white; padding: 0px; margin: 0px; border-image: none; }");

	//m_buttonBox->addButton(pbOpen, QDialogButtonBox::ActionRole);
	m_buttonBox->addButton(pbTest, QDialogButtonBox::ActionRole);
	//m_buttonBox->addButton(pbPreview, QDialogButtonBox::ActionRole);
	m_buttonBox->addButton(pbSave, QDialogButtonBox::ActionRole);
	m_buttonBox->addButton(pbReset, QDialogButtonBox::ActionRole);
	m_buttonBox->addButton(pbClose, QDialogButtonBox::ActionRole);

	// ...
	styleKeys = new QComboBox(this);
	styleKeys->setView(new QListView);
	connect(styleKeys, SIGNAL(currentIndexChanged(const QString&)), SLOT(onComboBoxSelectionChanged(const QString&)));
	gm = new QtGradientManager;
	setWindowTitle(tr("Edit Style Sheet"));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(m_buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(slotClicked(QAbstractButton*)));

	connect(m_editor, SIGNAL(textChanged()), this, SLOT(validateStyleSheet()));

	QToolBar *toolBar = new QToolBar(this);
	toolBar->setStyleSheet(styleSheet());

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(toolBar, 0, 0, 1, 2);
	layout->addWidget(m_editor, 1, 0, 1, 2);
	layout->addWidget(m_validityLabel, 2, 0, 1, 1);
	layout->addWidget(m_buttonBox, 2, 1, 1, 1);
	setLayout(layout);

	m_editor->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_editor, SIGNAL(customContextMenuRequested(QPoint)),
		this, SLOT(slotContextMenuRequested(QPoint)));

	//QSignalMapper *resourceActionMapper = new QSignalMapper(this);
	QSignalMapper *gradientActionMapper = new QSignalMapper(this);
	QSignalMapper *colorActionMapper = new QSignalMapper(this);

	gradientActionMapper->setMapping(m_addGradientAction, QString());
	colorActionMapper->setMapping(m_addColorAction, QString());

	connect(m_addGradientAction, SIGNAL(triggered()), gradientActionMapper, SLOT(map()));
	connect(m_addColorAction, SIGNAL(triggered()), colorActionMapper, SLOT(map()));
	connect(m_addFontAction, SIGNAL(triggered()), this, SLOT(slotAddFont()));

	const char * const colorProperties[] = {
		"color",
		"background-color",
		"alternate-background-color",
		"border-color",
		"border-top-color",
		"border-right-color",
		"border-bottom-color",
		"border-left-color",
		"gridline-color",
		"selection-color",
		"selection-background-color",
		0
	};

	QMenu *gradientActionMenu = new QMenu(this);
	QMenu *colorActionMenu = new QMenu(this);

	for (int colorProperty = 0; colorProperties[colorProperty]; ++colorProperty) {
		QAction *gradientAction = gradientActionMenu->addAction(QLatin1String(colorProperties[colorProperty]));
		QAction *colorAction = colorActionMenu->addAction(QLatin1String(colorProperties[colorProperty]));
		connect(gradientAction, SIGNAL(triggered()), gradientActionMapper, SLOT(map()));
		connect(colorAction, SIGNAL(triggered()), colorActionMapper, SLOT(map()));
		gradientActionMapper->setMapping(gradientAction, QLatin1String(colorProperties[colorProperty]));
		colorActionMapper->setMapping(colorAction, QLatin1String(colorProperties[colorProperty]));
	}

	connect(gradientActionMapper, SIGNAL(mapped(QString)), this, SLOT(slotAddGradient(QString)));
	connect(colorActionMapper, SIGNAL(mapped(QString)), this, SLOT(slotAddColor(QString)));

	m_addGradientAction->setMenu(gradientActionMenu);
	m_addColorAction->setMenu(colorActionMenu);

	toolBar->addAction(m_addGradientAction);
	toolBar->addAction(m_addColorAction);
	toolBar->addAction(m_addFontAction);
	toolBar->addWidget(styleKeys);
	setMinimumSize(640, 480);

	m_editor->setFocus();
	styleStorage = StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS);
	connect(styleStorage, SIGNAL(storageChanged()), SLOT(onStorageChanged()));
	fileSaved = true;
	onStorageChanged();
}

StyleSheetEditorDialog::~StyleSheetEditorDialog()
{
}

void StyleSheetEditorDialog::setOkButtonEnabled(bool v)
{
	pbPreview->setEnabled(v);
}

void StyleSheetEditorDialog::loadFile(const QString & fileName)
{
	QFile f(fileName);
	f.open(QIODevice::ReadOnly);
	f.seek(0);
	QByteArray ba = f.readAll();
	f.close();
	QString stylesheet = QString::fromLatin1(ba.data());
	setText(stylesheet);
	activeFileName = fileName;
	setWindowTitle(fileName + " - Style Sheet Editor");
	emit fileOpened(fileName);
	emit styleSheetChanged(stylesheet);
	fileSaved = true;
}

void StyleSheetEditorDialog::saveFile()
{
	QFile f(activeFileName);
	f.open(QIODevice::WriteOnly | QIODevice::Truncate);
	f.write(text().toLatin1());
}

void StyleSheetEditorDialog::slotContextMenuRequested(const QPoint &pos)
{
	QMenu *menu = m_editor->createStandardContextMenu();
	menu->addSeparator();
	menu->exec(mapToGlobal(pos));
	delete menu;
}

void StyleSheetEditorDialog::slotAddGradient(const QString &property)
{
	bool ok;
	const QGradient grad = QtGradientViewDialog::getGradient(&ok, gm, this);
	if (ok)
		insertCssProperty(property, QtGradientUtils::styleSheetCode(grad));
}

void StyleSheetEditorDialog::slotAddColor(const QString &property)
{
	const QColor color = QColorDialog::getColor(0xffffffff, this, QString(), QColorDialog::ShowAlphaChannel);
	if (!color.isValid())
		return;

	QString colorStr;

	if (color.alpha() == 255) {
		colorStr = QString(QLatin1String("rgb(%1, %2, %3)")).arg(
					color.red()).arg(color.green()).arg(color.blue());
	} else {
		colorStr = QString(QLatin1String("rgba(%1, %2, %3, %4)")).arg(
					color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
	}

	insertCssProperty(property, colorStr);
}

void StyleSheetEditorDialog::slotAddFont()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok, this);
	if (ok) {
		QString fontStr;
		if (font.weight() != QFont::Normal) {
			fontStr += QString::number(font.weight());
			fontStr += QLatin1Char(' ');
		}

		switch (font.style()) {
		case QFont::StyleItalic:
			fontStr += QLatin1String("italic ");
			break;
		case QFont::StyleOblique:
			fontStr += QLatin1String("oblique ");
			break;
		default:
			break;
		}
		fontStr += QString::number(font.pointSize());
		fontStr += QLatin1String("pt \"");
		fontStr += font.family();
		fontStr += QLatin1Char('"');

		insertCssProperty(QLatin1String("font"), fontStr);
		QString decoration;
		if (font.underline())
			decoration += QLatin1String("underline");
		if (font.strikeOut()) {
			if (!decoration.isEmpty())
				decoration += QLatin1Char(' ');
			decoration += QLatin1String("line-through");
		}
		insertCssProperty(QLatin1String("text-decoration"), decoration);
	}
}

void StyleSheetEditorDialog::insertCssProperty(const QString &name, const QString &value)
{
	if (!value.isEmpty()) {
		QTextCursor cursor = m_editor->textCursor();
		if (!name.isEmpty()) {
			cursor.beginEditBlock();
			cursor.removeSelectedText();
			cursor.movePosition(QTextCursor::EndOfLine);

			// Simple check to see if we're in a selector scope
			const QTextDocument *doc = m_editor->document();
			const QTextCursor closing = doc->find(QLatin1String("}"), cursor, QTextDocument::FindBackward);
			const QTextCursor opening = doc->find(QLatin1String("{"), cursor, QTextDocument::FindBackward);
			const bool inSelector = !opening.isNull() && (closing.isNull() ||
									  closing.position() < opening.position());
			QString insertion;
			if (m_editor->textCursor().block().length() != 1)
				insertion += QLatin1Char('\n');
			if (inSelector)
				insertion += QLatin1Char('\t');
			insertion += name;
			insertion += QLatin1String(": ");
			insertion += value;
			insertion += QLatin1Char(';');
			cursor.insertText(insertion);
			cursor.endEditBlock();
		} else {
			cursor.insertText(value);
		}
	}
}

void StyleSheetEditorDialog::slotClicked(QAbstractButton* button)
{
	QPushButton *pb = qobject_cast<QPushButton*>(button);
	if (pb == pbClose)
	{
		reject();
	}
	if (pb == pbOpen)
	{
		QString filename = QFileDialog::getOpenFileName(this, "Select QSS file", ".", "Qt Style Sheets (*.qss)");
		if (!filename.isEmpty())
		{
			loadFile(filename);
		}
	}
	if (pb == pbPreview)
	{
		styleStorage->previewStyle(text(), lastKey, 0);
	}
	if (pb == pbSave)
	{
		saveFile();
	}
	if (pb == pbReset)
	{
		styleStorage->previewReset();
	}
	if (pb == pbTest)
	{
		if (testForm->isVisible())
			testFormContainer->hide();
		else
			testFormContainer->show();
	}
}

void StyleSheetEditorDialog::onStorageChanged()
{
	styleKeys->clear();
	styleKeys->insertItems(0, styleStorage->fileKeys());
}

void StyleSheetEditorDialog::onComboBoxSelectionChanged(const QString& key)
{
	if (!fileSaved && key != lastKey)
	{
		QMessageBox::StandardButton btn = (QMessageBox::StandardButton)QMessageBox::question(this, "Save file?", "File changed. Do you want to save changes?", QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
		switch (btn)
		{
		case QMessageBox::Yes:
			saveFile();
			break;
		case QMessageBox::No:
			break;
		case QMessageBox::Cancel:
			styleKeys->setCurrentIndex(styleKeys->findText(lastKey));
			return;
			break;
		default:
			break;
		}
	}
	lastKey = key;
	qDebug() << key;
	QString fileName = styleStorage->fileFullName(key);
	qDebug() << fileName;
	loadFile(fileName);
}

QDialogButtonBox * StyleSheetEditorDialog::buttonBox() const
{
	return m_buttonBox;
}

QString StyleSheetEditorDialog::text() const
{
	return m_editor->toPlainText();
}

void StyleSheetEditorDialog::setText(const QString &t)
{
	m_editor->setText(t);
}

bool StyleSheetEditorDialog::isStyleSheetValid(const QString &styleSheet)
{
	QString correctStyleSheet = styleSheet;
	correctStyleSheet.replace("%IMAGES_PATH%", "/home/me");
	QCss::Parser parser(correctStyleSheet);
	QCss::StyleSheet sheet;
	if (parser.parse(&sheet))
		return true;
	QString fullSheet = QLatin1String("* { ");
	fullSheet += correctStyleSheet;
	fullSheet += QLatin1Char('}');
	QCss::Parser parser2(fullSheet);
	return parser2.parse(&sheet);
}

void StyleSheetEditorDialog::validateStyleSheet()
{
	fileSaved = false;
	const bool valid = isStyleSheetValid(m_editor->toPlainText());
	setOkButtonEnabled(valid);
	if (valid) {
		m_validityLabel->setText(tr("Valid Style Sheet"));
		m_validityLabel->setStyleSheet(QLatin1String("color: green"));
	} else {
		m_validityLabel->setText(tr("Invalid Style Sheet"));
		m_validityLabel->setStyleSheet(QLatin1String("color: red"));
	}
}
