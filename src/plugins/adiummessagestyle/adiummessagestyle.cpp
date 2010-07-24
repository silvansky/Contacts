#include "adiummessagestyle.h"

#include <QUrl>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QMimeData>
#include <QWebFrame>
#include <QByteArray>
#include <QClipboard>
#include <QStringList>
#include <QTextCursor>
#include <QWebSettings>
#include <QDomDocument>
#include <QApplication>
#include <QTextDocument>

#define SHARED_STYLE_PATH                   RESOURCES_DIR"/"RSR_STORAGE_ADIUMMESSAGESTYLES"/"STORAGE_SHARED_DIR
#define STYLE_CONTENTS_PATH                 "Contents"
#define STYLE_RESOURCES_PATH                STYLE_CONTENTS_PATH"/Resources"

#define APPEND_MESSAGE_WITH_SCROLL          "checkIfScrollToBottomIsNeeded(); appendCustumMessage(\"%1\",\"appendMessage\",%2,%3); scrollToBottomIfNeeded();"
#define APPEND_NEXT_MESSAGE_WITH_SCROLL     "checkIfScrollToBottomIsNeeded(); appendCustumMessage(\"%1\",\"appendNextMessage\",%2,%3); scrollToBottomIfNeeded();"
#define APPEND_MESSAGE                      "appendCustumMessage(\"%1\",\"appendMessage\",%2,%3);"
#define APPEND_NEXT_MESSAGE                 "appendCustumMessage(\"%1\",\"appendNextMessage\",%2,%3);"
#define APPEND_MESSAGE_NO_SCROLL            "appendCustumMessage(\"%1\",\"appendMessageNoScroll\",%2,%3);"
#define APPEND_NEXT_MESSAGE_NO_SCROLL       "appendCustumMessage(\"%1\",\"appendNextMessageNoScroll\",%2,%3);"
#define REPLACE_LAST_MESSAGE                "replaceLastMessage(\"%1\");"
#define DELETE_MESSAGE                      "deleteMessage(%1);"

#define TOPIC_MAIN_DIV	                    "<div id=\"topic\"></div>"
#define TOPIC_INDIVIDUAL_WRAPPER            "<span id=\"topicEdit\" ondblclick=\"this.setAttribute('contentEditable', true); this.focus();\">%1</span>"

#define CONSECUTIVE_TIMEOUT                 2*60

#define CAC_REPLACE                         0
#define CAC_INSERT_BEFORE                   1
#define CAC_INSERT_AFTER                    2

static const char *SenderColors[] =  {
	"blue", "blueviolet", "brown", "cadetblue", "chocolate", "coral", "cornflowerblue", "crimson",
	"darkblue", "darkcyan", "darkgoldenrod", "darkgreen", "darkmagenta", "darkolivegreen", "darkorange",
	"darkorchid", "darkred", "darksalmon", "darkslateblue", "darkslategrey", "darkturquoise", "darkviolet",
	"deeppink", "deepskyblue", "dodgerblue", "firebrick", "forestgreen", "fuchsia", "gold", "green",
	"hotpink", "indianred", "indigo", "lightcoral", "lightseagreen", "limegreen", "magenta", "maroon",
	"mediumblue", "mediumorchid", "mediumpurple", "mediumseagreen", "mediumslateblue", "mediumvioletred",
	"midnightblue", "navy", "olive", "olivedrab", "orange", "orangered", "orchid", "palevioletred", "peru",
	"purple", "red", "rosybrown", "royalblue", "saddlebrown", "salmon", "seagreen", "sienna", "slateblue",
	"steelblue", "teal", "tomato", "violet"
};

static int SenderColorsCount = sizeof(SenderColors)/sizeof(SenderColors[0]);

AdiumMessageStyle::AdiumMessageStyle(const QString &AStylePath, QObject *AParent) : QObject(AParent)
{
	FInfo = styleInfo(AStylePath);
	FVariants = styleVariants(AStylePath);
	FResourcePath = AStylePath+"/"STYLE_RESOURCES_PATH;
	initStyleSettings();
	loadTemplates();
	loadSenderColors();
	connect(AParent,SIGNAL(styleWidgetAdded(IMessageStyle *, QWidget *)),SLOT(onStyleWidgetAdded(IMessageStyle *, QWidget *)));
}

AdiumMessageStyle::~AdiumMessageStyle()
{

}

bool AdiumMessageStyle::isValid() const
{
	return !FIn_ContentHTML.isEmpty() && !styleId().isEmpty();
}

QString AdiumMessageStyle::styleId() const
{
	return FInfo.value(MSIV_NAME).toString();
}

QList<QWidget *> AdiumMessageStyle::styleWidgets() const
{
	return FWidgetStatus.keys();
}

QWidget *AdiumMessageStyle::createWidget(const IMessageStyleOptions &AOptions, QWidget *AParent)
{
	StyleViewer *view = new StyleViewer(AParent);
	changeOptions(view,AOptions,true);
	return view;
}

QString AdiumMessageStyle::senderColor(const QString &ASenderId) const
{
	if (!FSenderColors.isEmpty())
		return FSenderColors.at(qHash(ASenderId) % FSenderColors.count());
	return QString(SenderColors[qHash(ASenderId) % SenderColorsCount]);
}

QTextDocumentFragment AdiumMessageStyle::selection(QWidget *AWidget) const
{
	StyleViewer *view = qobject_cast<StyleViewer *>(AWidget);
	if (view && !view->page()->selectedText().isEmpty())
	{
		view->page()->triggerAction(QWebPage::Copy);
		return QTextDocumentFragment::fromHtml(QApplication::clipboard()->mimeData()->html());
	}
	return QTextDocumentFragment();
}

bool AdiumMessageStyle::changeOptions(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClean)
{
	StyleViewer *view = qobject_cast<StyleViewer *>(AWidget);
	if (view && AOptions.extended.value(MSO_STYLE_ID).toString()==styleId())
	{
		if (!FWidgetStatus.contains(AWidget))
		{
			connect(view,SIGNAL(linkClicked(const QUrl &)),SLOT(onLinkClicked(const QUrl &)));
			connect(view,SIGNAL(destroyed(QObject *)),SLOT(onStyleWidgetDestroyed(QObject *)));
			emit widgetAdded(AWidget);
		}

		if (AClean)
		{
			QString html = makeStyleTemplate(AOptions);
			fillStyleKeywords(html,AOptions);
			view->setHtml(html);
			FWidgetStatus[view].content.clear();
		}
		else
		{
			setVariant(AWidget,AOptions.extended.value(MSO_VARIANT).toString());
		}

		int fontSize = AOptions.extended.value(MSO_FONT_SIZE).toInt();
		QString fontFamily = AOptions.extended.value(MSO_FONT_FAMILY).toString();
		view->page()->settings()->setFontSize(QWebSettings::DefaultFontSize, fontSize!=0 ? fontSize : QWebSettings::globalSettings()->fontSize(QWebSettings::DefaultFontSize));
		view->page()->settings()->setFontFamily(QWebSettings::StandardFont, !fontFamily.isEmpty() ? fontFamily : QWebSettings::globalSettings()->fontFamily(QWebSettings::StandardFont));

		emit optionsChanged(AWidget,AOptions,AClean);
		return true;
	}
	return false;
}

QUuid AdiumMessageStyle::changeContent(QWidget *AWidget, const QString &AHtml, const IMessageContentOptions &AOptions)
{
	StyleViewer *view = FWidgetStatus.contains(AWidget) ? qobject_cast<StyleViewer *>(AWidget) : NULL;
	if (view)
	{
		int actionIndex = scriptActionIndex(AWidget,AOptions);
		if (actionIndex >= 0)
		{
			QUuid contentId;
			if (AOptions.action != IMessageContentOptions::Remove)
			{
				int actionCommand = scriptActionCommand(AOptions);
				bool sameSender = isSameSender(AWidget,AOptions,actionIndex);
				QString html = makeContentTemplate(AOptions,sameSender);
				fillContentKeywords(html,AOptions,sameSender);

				html.replace("%message%",processCommands(AHtml,AOptions));
				if (AOptions.kind == IMessageContentOptions::Topic)
					html.replace("%topic%",QString(TOPIC_INDIVIDUAL_WRAPPER).arg(AHtml));

				escapeStringForScript(html);
				QString script = scriptForAppendContent(AOptions,sameSender).arg(html).arg(actionIndex).arg(actionCommand);
				view->page()->mainFrame()->evaluateJavaScript(script);

				ContentParams cparams;
				cparams.kind = AOptions.kind;
				cparams.senderId = AOptions.senderId;
				cparams.time = AOptions.time;

				QList<ContentParams> &content = FWidgetStatus[AWidget].content;
				if (AOptions.action != IMessageContentOptions::Replace)
				{
					cparams.contentId = QUuid::createUuid();
					content.insert(actionIndex,cparams);
				}
				else
				{
					cparams.contentId = content.at(actionIndex).contentId;
					content.replace(actionIndex,cparams);
				}
				contentId = cparams.contentId;
			}
			else
			{
				view->page()->mainFrame()->evaluateJavaScript(QString(DELETE_MESSAGE).arg(actionIndex));

				QList<ContentParams> &content = FWidgetStatus[AWidget].content;
				contentId = content.value(actionIndex).contentId;
				content.removeAt(actionIndex);
			}
			emit contentChanged(AWidget,contentId,AHtml,AOptions);
			return contentId;
		}
	}
	return QUuid();
}

int AdiumMessageStyle::version() const
{
	return FInfo.value(MSIV_VERSION,0).toInt();
}

QMap<QString, QVariant> AdiumMessageStyle::infoValues() const
{
	return FInfo;
}

QList<QString> AdiumMessageStyle::variants() const
{
	return FVariants;
}

QList<QString> AdiumMessageStyle::styleVariants(const QString &AStylePath)
{
	QList<QString> files;
	if (!AStylePath.isEmpty())
	{
		QDir dir(AStylePath+"/"STYLE_RESOURCES_PATH"/Variants");
		files = dir.entryList(QStringList("*.css"),QDir::Files,QDir::Name);
		for (int i=0; i<files.count();i++)
			files[i].chop(4);
	}
	return files;
}

QMap<QString, QVariant> AdiumMessageStyle::styleInfo(const QString &AStylePath)
{
	QMap<QString, QVariant> info;

	QFile file(AStylePath+"/"STYLE_CONTENTS_PATH"/Info.plist");
	if (!AStylePath.isEmpty() && file.open(QFile::ReadOnly))
	{
		QDomDocument doc;
		if (doc.setContent(file.readAll(),true))
		{
			QDomElement elem = doc.documentElement().firstChildElement("dict").firstChildElement("key");
			while (!elem.isNull())
			{
				QString key = elem.text();
				if (!key.isEmpty())
				{
					elem = elem.nextSiblingElement();
					if (elem.tagName() == "string")
						info.insert(key,elem.text());
					else if (elem.tagName() == "integer")
						info.insert(key,elem.text().toInt());
					else if (elem.tagName() == "true")
						info.insert(key,true);
					else if (elem.tagName() == "false")
						info.insert(key,false);
				}
				elem = elem.nextSiblingElement("key");
			}
		}
	}
	return info;
}

int AdiumMessageStyle::scriptActionCommand(const IMessageContentOptions &AOptions) const
{
	int command = -1;
	if (AOptions.action == IMessageContentOptions::InsertAfter)
		command = CAC_INSERT_BEFORE;
	else if (AOptions.action == IMessageContentOptions::InsertBefore)
		command = CAC_INSERT_BEFORE;
	else if (AOptions.action == IMessageContentOptions::Replace)
		command = CAC_REPLACE;
	return command;
}

int AdiumMessageStyle::scriptActionIndex(QWidget *AWidget, const IMessageContentOptions &AOptions) const
{
	int index = -1;
	if (!AOptions.contentId.isNull() || AOptions.time.isValid())
	{
		const QList<ContentParams> &content = FWidgetStatus.value(AWidget).content;
		for (index=content.count()-1; index>=0; index--)
		{
			if (!AOptions.contentId.isNull() && AOptions.contentId==content.at(index).contentId)
				break;
			else if (AOptions.time.isValid() && AOptions.time>=content.at(index).time)
				break;
		}

		if (index >= 0)
		{
			if (AOptions.contentId == content.at(index).contentId)
			{
				if (AOptions.action == IMessageContentOptions::InsertAfter)
				{
					index++;
				}
			}
			else
			{
				index++;
			}
		}
		else if (AOptions.contentId.isNull() && AOptions.time.isValid())
		{
			if (AOptions.action == IMessageContentOptions::InsertAfter)
			{
				index = 0;
			}
			else if (AOptions.action == IMessageContentOptions::InsertBefore)
			{
				index = 0;
			}
		}
	}
	return index;
}

bool AdiumMessageStyle::isSameSender(QWidget *AWidget, const IMessageContentOptions &AOptions, int AIndex) const
{
	if (!FCombineConsecutive)
		return false;
	if (AOptions.kind!=IMessageContentOptions::Message)
		return false;
	if (AOptions.senderId.isEmpty())
		return false;
	if (AIndex <= 0)
		return false;

	const WidgetStatus &wstatus = FWidgetStatus.value(AWidget);
	if (wstatus.content.isEmpty())
		return false;

	const ContentParams &cparams = AIndex<wstatus.content.count() ? wstatus.content.at(AIndex-1) : wstatus.content.last();
	if (cparams.kind != AOptions.kind)
		return false;
	if (cparams.senderId != AOptions.senderId)
		return false;
	if (cparams.time.secsTo(AOptions.time) > CONSECUTIVE_TIMEOUT)
		return false;

	return true;
}

void AdiumMessageStyle::setVariant(QWidget *AWidget, const QString &AVariant)
{
	StyleViewer *view = FWidgetStatus.contains(AWidget) ? qobject_cast<StyleViewer *>(AWidget) : NULL;
	if (view)
	{
		QString variant = QDir::cleanPath(QString("Variants/%1.css").arg(!FVariants.contains(AVariant) ? FInfo.value(MSIV_DEFAULT_VARIANT,"../main").toString() : AVariant));
		escapeStringForScript(variant);
		QString script = QString("setStylesheet(\"%1\",\"%2\");").arg("mainStyle").arg(variant);
		view->page()->mainFrame()->evaluateJavaScript(script);
	}
}

QString AdiumMessageStyle::makeStyleTemplate(const IMessageStyleOptions &AOptions) const
{
	bool usingCustomTemplate = true;
	QString htmlFileName = FResourcePath+"/Template.html";
	if (!QFile::exists(htmlFileName))
	{
		usingCustomTemplate = false;
		htmlFileName = qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Template.html";
	}

	QString html = loadFileData(htmlFileName,QString::null);
	if (!html.isEmpty())
	{
		static QString extendScripts = loadFileData(qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Extension.js",QString::null);
		html.insert(html.indexOf("<script ",0,Qt::CaseInsensitive),QString("<script type='text/javascript'>%1</script>").arg(extendScripts));

		QString headerHTML;
		if (AOptions.extended.value(MSO_HEADER_TYPE).toInt() == AdiumMessageStyle::HeaderTopic)
			headerHTML = TOPIC_MAIN_DIV;
		else if (AOptions.extended.value(MSO_HEADER_TYPE).toInt() == AdiumMessageStyle::HeaderNormal)
			headerHTML =  loadFileData(FResourcePath+"/Header.html",QString::null);
		QString footerHTML = loadFileData(FResourcePath+"/Footer.html",QString::null);

		QString variant = AOptions.extended.value(MSO_VARIANT).toString();
		if (!FVariants.contains(variant))
			variant = FInfo.value(MSIV_DEFAULT_VARIANT,"../main").toString();
		variant = QDir::cleanPath(QString("Variants/%1.css").arg(variant));

		html.replace(html.indexOf("%@"),2,QUrl::fromLocalFile(FResourcePath).toString()+"/");
		if (!usingCustomTemplate || version()>=3)
			html.replace(html.indexOf("%@"),2, version()>=3 ? "@import url( \"main.css\" );" : "");
		html.replace(html.indexOf("%@"),2,variant);
		html.replace(html.indexOf("%@"),2,headerHTML);
		html.replace(html.indexOf("%@"),2,footerHTML);
	}
	return html;
}

void AdiumMessageStyle::fillStyleKeywords(QString &AHtml, const IMessageStyleOptions &AOptions) const
{
	AHtml.replace("%chatName%",AOptions.extended.value(MSO_CHAT_NAME).toString());
	AHtml.replace("%timeOpened%",Qt::escape(AOptions.extended.value(MSO_START_DATE_TIME).toDateTime().time().toString()));
	AHtml.replace("%dateOpened%",Qt::escape(AOptions.extended.value(MSO_START_DATE_TIME).toDateTime().date().toString()));
	AHtml.replace("%sourceName%",AOptions.extended.value(MSO_ACCOUNT_NAME).toString());
	AHtml.replace("%destinationName%",AOptions.extended.value(MSO_CHAT_NAME).toString());
	AHtml.replace("%destinationDisplayName%",AOptions.extended.value(MSO_CHAT_NAME).toString());
	AHtml.replace("%outgoingIconPath%",AOptions.extended.value(MSO_SELF_AVATAR,"outgoing_icon.png").toString());
	AHtml.replace("%incomingIconPath%",AOptions.extended.value(MSO_CONTACT_AVATAR,"incoming_icon.png").toString());
	AHtml.replace("%outgoingColor%",AOptions.extended.value(MSO_SELF_COLOR).toString());
	AHtml.replace("%incomingColor%",AOptions.extended.value(MSO_CONTACT_COLOR).toString());
	AHtml.replace("%serviceIconPath%", AOptions.extended.value(MSO_SERVICE_ICON_PATH).toString());
	AHtml.replace("%serviceIconImg%", QString("<img class=\"serviceIcon\" src=\"%1\">")
	              .arg(AOptions.extended.value(MSO_SERVICE_ICON_PATH,"outgoing_icon.png").toString()));

	QString background;
	if (FAllowCustomBackground)
	{
		if (!AOptions.extended.value(MSO_BG_IMAGE_FILE).toString().isEmpty())
		{
			int imageLayout = AOptions.extended.value(MSO_BG_IMAGE_LAYOUT).toInt();
			if (imageLayout == ImageNormal)
				background.append("background-image: url('%1'); background-repeat: no-repeat; background-attachment:fixed;");
			else if (imageLayout == ImageCenter)
				background.append("background-image: url('%1'); background-position: center; background-repeat: no-repeat; background-attachment:fixed;");
			else if (imageLayout == ImageTitle)
				background.append("background-image: url('%1'); background-repeat: repeat;");
			else if (imageLayout == ImageTitleCenter)
				background.append("background-image: url('%1'); background-repeat: repeat; background-position: center;");
			else if (imageLayout == ImageScale)
				background.append("background-image: url('%1'); -webkit-background-size: 100% 100%; background-size: 100% 100%; background-attachment: fixed;");
			background = background.arg(AOptions.extended.value(MSO_BG_IMAGE_FILE).toString());
		}
		if (!AOptions.extended.value(MSO_BG_COLOR).toString().isEmpty())
		{
			QColor color(AOptions.extended.value(MSO_BG_COLOR).toString());
			if (!color.isValid())
				color.setNamedColor("#"+AOptions.extended.value(MSO_BG_COLOR).toString());
			if (color.isValid())
			{
				int r,g,b,a;
				color.getRgb(&r,&g,&b,&a);
				background.append(QString("background-color: rgba(%1, %2, %3, %4);").arg(r).arg(g).arg(b).arg(qreal(a)/255.0));
			}
		}
	}
	AHtml.replace("==bodyBackground==", background);
}

QString AdiumMessageStyle::makeContentTemplate(const IMessageContentOptions &AOptions, bool ASameSender) const
{
	QString html;
	if (false && !FTopicHTML.isEmpty() && AOptions.kind==IMessageContentOptions::Topic)
	{
		html = FTopicHTML;
	}
	else if (!FStatusHTML.isEmpty() && AOptions.kind==IMessageContentOptions::Status)
	{
		html = FStatusHTML;
	}
	else if (AOptions.type & IMessageContentOptions::History)
	{
		if (AOptions.direction == IMessageContentOptions::DirectionIn)
			html = ASameSender ? FIn_NextContextHTML : FIn_ContextHTML;
		else
			html = ASameSender ? FOut_NextContextHTML : FOut_ContextHTML;
	}
	else if (AOptions.direction == IMessageContentOptions::DirectionIn)
	{
		html = ASameSender ? FIn_NextContentHTML : FIn_ContentHTML;
	}
	else
	{
		html = ASameSender ? FOut_NextContentHTML : FOut_ContentHTML;
	}
	
	if (AOptions.extensions & IMessageContentOptions::Unreaded)
	{
		QString templ;
		if (AOptions.direction == IMessageContentOptions::DirectionIn)
			templ = ASameSender ? FIn_NextUnreadHTML : FIn_UnreadHTML;
		else
			templ = ASameSender ? FOut_NextUnreadHTML : FOut_UnreadHTML;
		html = templ.replace("%html%",html);
	}

	if (AOptions.extensions & IMessageContentOptions::Offline)
	{
		QString templ;
		if (AOptions.direction == IMessageContentOptions::DirectionIn)
			templ = ASameSender ? FIn_NextOfflineHTML : FIn_OfflineHTML;
		else
			templ = ASameSender ? FOut_NextOfflineHTML : FOut_OfflineHTML;
		html = templ.replace("%html%",html);
	}

	return html;
}

void AdiumMessageStyle::fillContentKeywords(QString &AHtml, const IMessageContentOptions &AOptions, bool ASameSender) const
{
	bool isDirectionIn = AOptions.direction == IMessageContentOptions::DirectionIn;

	QStringList messageClasses;
	if (FCombineConsecutive && ASameSender)
		messageClasses << MSMC_CONSECUTIVE;

	if (AOptions.kind == IMessageContentOptions::Status)
		messageClasses << MSMC_STATUS;
	else
		messageClasses << MSMC_MESSAGE;

	if (AOptions.type & IMessageContentOptions::Groupchat)
		messageClasses << MSMC_GROUPCHAT;
	if (AOptions.type & IMessageContentOptions::History)
		messageClasses << MSMC_HISTORY;
	if (AOptions.type & IMessageContentOptions::Event)
		messageClasses << MSMC_EVENT;
	if (AOptions.type & IMessageContentOptions::Mention)
		messageClasses << MSMC_MENTION;
	if (AOptions.type & IMessageContentOptions::Notification)
		messageClasses << MSMC_NOTIFICATION;
	if (AOptions.type & IMessageContentOptions::DateSeparator)
		messageClasses << MSSK_DATE_SEPARATOR;

	if (isDirectionIn)
		messageClasses << MSMC_INCOMING;
	else
		messageClasses << MSMC_OUTGOING;

	AHtml.replace("%messageClasses%", messageClasses.join(" "));

	//AHtml.replace("%messageDirection%", AOptions.isAlignLTR ? "ltr" : "rtl" );
	AHtml.replace("%senderStatusIcon%",AOptions.senderIcon);
	AHtml.replace("%shortTime%", Qt::escape(AOptions.time.toString(tr("hh:mm"))));
	AHtml.replace("%service%","");

	QString avatar = AOptions.senderAvatar;
	if (!QFile::exists(avatar))
	{
		avatar = isDirectionIn ? "Incoming/buddy_icon.png" : "Outgoing/buddy_icon.png";
		if (!isDirectionIn && !QFile::exists(avatar))
			avatar = "Incoming/buddy_icon.png";
	}
	AHtml.replace("%userIconPath%",avatar);

	QString timeFormat = !AOptions.timeFormat.isEmpty() ? AOptions.timeFormat : tr("hh:mm:ss");
	QString time = Qt::escape(AOptions.time.toString(timeFormat));
	AHtml.replace("%time%", time);

	QRegExp timeRegExp("%time\\{([^}]*)\\}%");
	for (int pos=0; pos!=-1; pos = timeRegExp.indexIn(AHtml, pos))
		if (!timeRegExp.cap(0).isEmpty())
			AHtml.replace(pos, timeRegExp.cap(0).length(), time);

	QString sColor = !AOptions.senderColor.isEmpty() ? AOptions.senderColor : senderColor(AOptions.senderId);
	AHtml.replace("%senderColor%",sColor);

	QRegExp scolorRegExp("%senderColor\\{([^}]*)\\}%");
	for (int pos=0; pos!=-1; pos = scolorRegExp.indexIn(AHtml, pos))
		if (!scolorRegExp.cap(0).isEmpty())
			AHtml.replace(pos, scolorRegExp.cap(0).length(), sColor);

	if (AOptions.kind == IMessageContentOptions::Status)
	{
		AHtml.replace("%status%","");
		AHtml.replace("%statusSender%",AOptions.senderName);
	}
	else
	{
		AHtml.replace("%senderScreenName%",AOptions.senderId);
		AHtml.replace("%sender%",AOptions.senderName);
		AHtml.replace("%senderDisplayName%",AOptions.senderName);
		AHtml.replace("%senderPrefix%","");

		QString rgbaColor;
		QColor bgColor(AOptions.textBGColor);
		QRegExp colorRegExp("%textbackgroundcolor\\{([^}]*)\\}%");
		for (int pos=0; pos!=-1; pos = colorRegExp.indexIn(AHtml, pos))
		{
			if (!colorRegExp.cap(0).isEmpty())
			{
				if (bgColor.isValid())
				{
					int r,g,b;
					bool ok = false;
					qreal a = colorRegExp.cap(1).toDouble(&ok);
					bgColor.setAlphaF(ok ? a : 1.0);
					bgColor.getRgb(&r,&g,&b);
					rgbaColor = QString("rgba(%1, %2, %3, %4)").arg(r).arg(g).arg(b).arg(a);
				}
				else if (rgbaColor.isEmpty())
				{
					rgbaColor = "inherit";
				}
				AHtml.replace(pos, colorRegExp.cap(0).length(), rgbaColor);
			}
		}
	}
}

QString AdiumMessageStyle::processCommands(const QString &AHtml, const IMessageContentOptions &AOptions) const
{
	bool changed = false;
	QTextDocument message;
	message.setHtml(AHtml);

	// "/me" command
	if (!AOptions.senderName.isEmpty())
	{
		QRegExp me("^/me\\s");
		for (QTextCursor cursor = message.find(me); !cursor.isNull();  cursor = message.find(me,cursor))
		{
			changed = true;
			cursor.insertHtml("*&nbsp;<i>"+AOptions.senderName+"&nbsp;</i>");
		}
	}

	if (changed)
	{
		QString html = message.toHtml();
		QRegExp body("<body.*>(.*)</body>");
		body.setMinimal(false);
		return html.indexOf(body)>=0 ? body.cap(1).trimmed() : html;
	}

	return AHtml;
}

void AdiumMessageStyle::escapeStringForScript(QString &AText) const
{
	AText.replace("\\","\\\\");
	AText.replace("\"","\\\"");
	AText.replace("\n","");
	AText.replace("\r","<br>");
}

QString AdiumMessageStyle::scriptForAppendContent(const IMessageContentOptions &AOptions, bool ASameSender) const
{
	QString script;

	if (version() >= 4)
	{
		if (AOptions.noScroll)
			script = (FCombineConsecutive && ASameSender ? APPEND_NEXT_MESSAGE_NO_SCROLL : APPEND_MESSAGE_NO_SCROLL);
		else
			script = (FCombineConsecutive && ASameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
	}
	else if (version() >= 3)
	{
		if (AOptions.noScroll)
			script = (FCombineConsecutive && ASameSender ? APPEND_NEXT_MESSAGE_NO_SCROLL : APPEND_MESSAGE_NO_SCROLL);
		else
			script = (FCombineConsecutive && ASameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
	}
	else if (version() >= 1)
	{
		script = (ASameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
	}
	else
	{
		script = (ASameSender ? APPEND_NEXT_MESSAGE_WITH_SCROLL : APPEND_MESSAGE_WITH_SCROLL);
	}
	return script;
}

QString AdiumMessageStyle::loadFileData(const QString &AFileName, const QString &DefValue) const
{
	if (QFile::exists(AFileName))
	{
		QFile file(AFileName);
		if (file.open(QFile::ReadOnly))
		{
			QByteArray html = file.readAll();
			return QString::fromUtf8(html.data(),html.size());
		}
	}
	return DefValue;
}

void AdiumMessageStyle::loadTemplates()
{
	FStatusHTML =          loadFileData(FResourcePath+"/Status.html",FIn_ContentHTML);
	FTopicHTML =           loadFileData(FResourcePath+"/Topic.html",QString::null);

	FIn_ContentHTML =      loadFileData(FResourcePath+"/Incoming/Content.html",QString::null);
	FIn_NextContentHTML =  loadFileData(FResourcePath+"/Incoming/NextContent.html",FIn_ContentHTML);
	
	FIn_ContextHTML =      loadFileData(FResourcePath+"/Incoming/Context.html",FIn_ContentHTML);
	FIn_NextContextHTML =  loadFileData(FResourcePath+"/Incoming/NextContext.html",FIn_NextContentHTML);
	
	FOut_ContentHTML =     loadFileData(FResourcePath+"/Outgoing/Content.html",FIn_ContentHTML);
	FOut_NextContentHTML = loadFileData(FResourcePath+"/Outgoing/NextContent.html",FOut_ContentHTML);

	FOut_ContextHTML =     loadFileData(FResourcePath+"/Outgoing/Context.html",FOut_ContentHTML);
	FOut_NextContextHTML = loadFileData(FResourcePath+"/Outgoing/NextContext.html",FOut_NextContentHTML);

	FIn_UnreadHTML =       loadFileData(qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Incoming/Unread.html", QString::null);
	FIn_UnreadHTML =       loadFileData(FResourcePath+"/Incoming/Unread.html",FIn_UnreadHTML);
	FIn_NextUnreadHTML =   loadFileData(qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Incoming/NextUnread.html", FIn_UnreadHTML);
	FIn_NextUnreadHTML =   loadFileData(FResourcePath+"/Incoming/NextUnread.html",FIn_NextUnreadHTML);

	FIn_OfflineHTML =      loadFileData(qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Incoming/Offline.html", QString::null);
	FIn_OfflineHTML =      loadFileData(FResourcePath+"/Incoming/Offline.html",FIn_OfflineHTML);
	FIn_NextOfflineHTML =  loadFileData(qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Incoming/NextOffline.html", FIn_OfflineHTML);
	FIn_NextOfflineHTML =  loadFileData(FResourcePath+"/Incoming/NextOffline.html",FIn_NextOfflineHTML);

	FOut_UnreadHTML =      loadFileData(qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Outgoing/Unread.html", FIn_UnreadHTML);
	FOut_UnreadHTML =      loadFileData(FResourcePath+"/Outgoing/Unread.html",FOut_UnreadHTML);
	FOut_NextUnreadHTML =  loadFileData(qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Outgoing/NextUnread.html", FOut_UnreadHTML);
	FOut_NextUnreadHTML =  loadFileData(FResourcePath+"/Outgoing/NextUnread.html",FOut_NextUnreadHTML);

	FOut_OfflineHTML =     loadFileData(qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Outgoing/Offline.html", FIn_OfflineHTML);
	FOut_OfflineHTML =     loadFileData(FResourcePath+"/Outgoing/Offline.html",FOut_OfflineHTML);
	FOut_NextOfflineHTML = loadFileData(qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Outgoing/NextOffline.html", FOut_OfflineHTML);
	FOut_NextOfflineHTML = loadFileData(FResourcePath+"/Outgoing/NextOffline.html",FOut_NextOfflineHTML);
}

void AdiumMessageStyle::loadSenderColors()
{
	QFile colors(FResourcePath + "/Incoming/SenderColors.txt");
	if (colors.open(QFile::ReadOnly))
		FSenderColors = QString::fromUtf8(colors.readAll()).split(':',QString::SkipEmptyParts);
}

void AdiumMessageStyle::initStyleSettings()
{
	FCombineConsecutive = !FInfo.value(MSIV_DISABLE_COMBINE_CONSECUTIVE,false).toBool();
	FAllowCustomBackground = !FInfo.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool();
}

void AdiumMessageStyle::onLinkClicked(const QUrl &AUrl)
{
	StyleViewer *view = qobject_cast<StyleViewer *>(sender());
	emit urlClicked(view,AUrl);
}

void AdiumMessageStyle::onStyleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget)
{
	if (AStyle!=this && FWidgetStatus.contains(AWidget))
	{
		FWidgetStatus.remove(AWidget);
		emit widgetRemoved(AWidget);
	}
}

void AdiumMessageStyle::onStyleWidgetDestroyed(QObject *AObject)
{
	FWidgetStatus.remove((QWidget *)AObject);
	emit widgetRemoved((QWidget *)AObject);
}
