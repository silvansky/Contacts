#include "localeoptionswidget.h"

#include <QDir>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>
#include <QApplication>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <utils/menu.h>
#include <utils/options.h>
#include <utils/stylestorage.h>

#define SVN_LOCALE_NAME             "Locale"
#define DEFAULT_LOCALE              QString("en")
#define ADR_LOCALE_NAME             Action::DR_Parametr1

LocaleOptionsWidget::LocaleOptionsWidget(QWidget *AParent) : QWidget(AParent)
{
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_OPTIONS_LOCALE_OPTIONS_WIDGET);

	QMap<QString,QString> sortedLocales;
	QDir tsDir(QApplication::applicationDirPath());
	tsDir.cd(TRANSLATIONS_DIR);
	foreach(QString localeName, tsDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot)<<DEFAULT_LOCALE)
	{
		QString locale = localeString(localeName);
		if (!locale.isEmpty() && !sortedLocales.values().contains(localeName))
			sortedLocales.insert(locale,localeName);
	}

	QLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(26,0,0,0);
	for (QMap<QString,QString>::const_iterator it=sortedLocales.constBegin(); it!=sortedLocales.constEnd(); it++)
	{
		QRadioButton *button = new QRadioButton(this);
		button->setText(it.key());
		layout->addWidget(button);
		FLocales.insert(button,it.value());
	}

	QLabel *label = new QLabel(tr("*Language settings will be applied on next application launch"),this);
	layout->addWidget(label);

	reset();
}

LocaleOptionsWidget::~LocaleOptionsWidget()
{

}

void LocaleOptionsWidget::apply()
{
	Options::setGlobalValue(SVN_LOCALE_NAME,selectedLocale());
	emit childApply();
}

void LocaleOptionsWidget::reset()
{
	setSelectedLocale(Options::globalValue(SVN_LOCALE_NAME).toString());
	emit childReset();
}

QString LocaleOptionsWidget::selectedLocale() const
{
	for (QMap<QRadioButton *,QString>::const_iterator it=FLocales.constBegin(); it!=FLocales.constEnd(); it++)
	{
		if (it.key()->isChecked())
			return it.value();
	}
	return QString("en");
}

void LocaleOptionsWidget::setSelectedLocale(const QString &ALocaleName)
{
	QRadioButton *button = FLocales.key(ALocaleName);
	if (button==NULL && ALocaleName.contains("_"))
		button = FLocales.key(ALocaleName.split("_").value(0));
	if (button==NULL && ALocaleName.isEmpty())
		button = FLocales.key(QLocale().name());
	if (button==NULL && ALocaleName.isEmpty())
		button = FLocales.key(QLocale().name().split("_").value(0));
	if (button==NULL)
		button = FLocales.key(DEFAULT_LOCALE);
	if (button)
		button->setChecked(true);
}

QString LocaleOptionsWidget::localeString(const QString &ALocaleName) const
{
	if (!ALocaleName.isEmpty())
	{
		QLocale locale(ALocaleName);
		if (locale.language() != QLocale::C)
		{
#if QT_VERSION < QT_VERSION_CHECK(4,8,0)
			QString str = QString("%1 (%2)").arg(QLocale::languageToString(locale.language()),QLocale::countryToString(locale.country()));
#else
			QString str = QString("%1 (%2)").arg(locale.nativeLanguageName(),locale.nativeCountryName());
#endif
			if (!str.isEmpty())
				str[0] = str[0].toUpper();
			return str;
		}
		return QString::null;
	}
	return tr("System language");
}
