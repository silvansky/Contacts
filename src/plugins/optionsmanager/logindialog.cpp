#include "logindialog.h"

#include <QDir>
#include <QFile>
#include <QPainter>
#include <QKeyEvent>
#include <QListView>
#include <QCompleter>
#include <QTextCursor>
#include <QDomDocument>
#include <QItemDelegate>
#include <QTextDocument>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QAbstractTextDocumentLayout>
#include <utils/customborderstorage.h>
#include <definitions/customborder.h>
#include <utils/graphicseffectsstorage.h>
#include <definitions/resources.h>
#include <definitions/graphicseffects.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>
#include <utils/log.h>
#include <utils/customlistview.h>
#include <utils/custominputdialog.h>

#ifdef Q_WS_WIN32
#	include <windows.h>
#elif defined Q_WS_X11
#	include <X11/XKBlib.h>
#	undef KeyPress
#	undef FocusIn
#	undef FocusOut
#endif

#define FILE_LOGIN		"login.xml"

enum ConnectionSettings {
	CS_DEFAULT,
	CS_IE_PROXY,
	CS_FF_PROXY,
	CS_COUNT
};

class CompleterDelegate :
		public QItemDelegate
{
public:
	CompleterDelegate(QObject *AParent): QItemDelegate(AParent) {}
	QSize drawIndex(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
	{
		QStyleOptionViewItemV4 option = QItemDelegate::setOptions(AIndex, AOption);

		if (APainter)
		{
			APainter->save();
			APainter->setClipRect(option.rect);
			QItemDelegate::drawBackground(APainter,option,AIndex);
		}

		Jid streamJid = AIndex.data(Qt::DisplayRole).toString();
		bool isSelected = option.state & QStyle::State_Selected;

		QTextDocument doc;
		doc.setDefaultFont(option.font);
		QTextCursor cursor(&doc);

		QTextCharFormat nodeFormat = cursor.charFormat();
		nodeFormat.setForeground(option.palette.brush(QPalette::Normal, isSelected ? QPalette::HighlightedText : QPalette::Text));
		cursor.insertText(streamJid.node(),nodeFormat);

		QTextCharFormat domainFormat = cursor.charFormat();
		domainFormat.setForeground(option.palette.brush(QPalette::Disabled, isSelected ? QPalette::HighlightedText : QPalette::Text));
		cursor.insertText("@",domainFormat);
		cursor.insertText(streamJid.domain(),domainFormat);

		if (APainter)
		{
			QAbstractTextDocumentLayout::PaintContext context;
			context.palette = option.palette;
			QRect rect = option.rect;
			rect.moveLeft(rect.left() + 6);
			// TODO: vertically center text in option.rect
			rect.moveTop(rect.top() + 2);
			APainter->translate(rect.topLeft());
			doc.documentLayout()->draw(APainter, context);
			APainter->restore();
		}

		return doc.documentLayout()->documentSize().toSize();
	}
	virtual void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
	{
		drawIndex(APainter,AOption,AIndex);
	}
	virtual QSize sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
	{
		QSize hint = drawIndex(NULL,AOption,AIndex);
		//hint.setWidth(80);
		hint.setHeight(27);
		return hint;
	}
};

class DomainComboDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	DomainComboDelegate(QObject *parent, QComboBox *cmb) : QStyledItemDelegate(parent), mCombo(cmb) {}

	static bool isSeparator(const QModelIndex &index)
	{
		return index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator");
	}

	static void setSeparator(QAbstractItemModel *model, const QModelIndex &index)
	{
		model->setData(index, QString::fromLatin1("separator"), Qt::AccessibleDescriptionRole);
		if (QStandardItemModel *m = qobject_cast<QStandardItemModel*>(model))
			if (QStandardItem *item = m->itemFromIndex(index))
				item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
	}

protected:
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		if (isSeparator(index))
		{
			QRect rect = option.rect;
			if (const QStyleOptionViewItemV3 *v3 = qstyleoption_cast<const QStyleOptionViewItemV3*>(&option))
				if (const QAbstractItemView *view = qobject_cast<const QAbstractItemView*>(v3->widget))
					rect.setWidth(view->viewport()->width());
			QImage separator = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_MENUSEPARATOR);
			painter->fillRect(rect, QBrush(separator));
		}
		else
		{
			QStyledItemDelegate::paint(painter, option, index);
		}
	}

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		if (isSeparator(index))
		{
			QImage separator = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_MENUSEPARATOR);
			int pm = mCombo->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, mCombo);
			return QSize(pm, separator.height());
		}
		return QStyledItemDelegate::sizeHint(option, index);
	}
private:
	QComboBox *mCombo;
};

LoginDialog::LoginDialog(IPluginManager *APluginManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
//	ui.lblRegister->setProperty("ignoreFilter", true);
//	ui.lblHelp->setProperty("ignoreFilter", true);
//	ui.lblConnectSettings->setProperty("ignoreFilter", true);
//	ui.lblForgotPassword->setProperty("ignoreFilter", true);
	domainPrevIndex = 0;
	ui.lneNode->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lnePassword->setAttribute(Qt::WA_MacShowFocusRect, false);
	connect(ui.chbShowPassword, SIGNAL(stateChanged(int)), SLOT(onShowPasswordToggled(int)));
	FConnectionErrorWidget = new QWidget;
	FConnectionErrorWidget->setObjectName("connectionErrorWidget");
	QVBoxLayout * vlayout = new QVBoxLayout;
	vlayout->setSpacing(4);
	vlayout->setContentsMargins(0, 0, 0, 0);
	vlayout->addWidget(ui.lblConnectError);
	vlayout->addWidget(ui.lblConnectSettings);
	vlayout->addWidget(ui.lblXmppError);
	vlayout->addWidget(ui.lblReconnect);
	vlayout->addWidget(ui.chbShowPassword);
	FConnectionErrorWidget->setLayout(vlayout);
	ui.lneNode->setProperty("error", false);
	ui.lnePassword->setProperty("error", false);
	ui.cmbDomain->setProperty("error", false);
	ui.tlbDomain->setProperty("error", false);

	domainsMenu = new Menu(this);
	domainsMenu->setObjectName("domainsMenu");
	ui.tlbDomain->setMenu(domainsMenu);

	//	ui.frmDomain->setProperty("error", false);
	ui.cmbDomain->setView(new QListView());
	ui.cmbDomain->view()->setItemDelegate(new DomainComboDelegate(ui.cmbDomain->view(), ui.cmbDomain));
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose, true);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_OPTIONS_LOGINDIALOG);
	connect(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS), SIGNAL(stylePreviewReset()), SLOT(onStylePreviewReset()));
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);
	FConnectionErrorWidget->setStyleSheet(styleSheet());

	FNewProfile = true;
	FSavedPasswordCleared = false;
	FConnectionSettings = CS_DEFAULT;
	initialize(APluginManager);
	FOptionsManager->setCurrentProfile(QString::null,QString::null);

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblLogo,MNI_OPTIONS_LOGIN_LOGO,0,0,"pixmap");
	ui.lblLogo->setFixedHeight(43);
	//ui.lblLogo->setText(QString("<font size=+3>%1</font>").arg(tr("Contacts")));

	ui.lblRegister->setText(tr("Enter your Rambler login and password, or %1.")
				.arg("<a href='http://id.rambler.ru/script/newuser.cgi'><span style=' font-size:9pt; text-decoration: underline; color:#ffffff;'>%1</span></a>")
				.arg(tr("register")));
	ui.lblForgotPassword->setText(QString("<a href='http://id.rambler.ru/script/reminder.cgi'><span style='font-size:9pt; text-decoration: underline; color:#acacac;'>%1</span></a>")
				      .arg(tr("Forgot your password?")));

	connect(ui.lblRegister,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblForgotPassword,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblConnectSettings,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	ui.lblConnectSettings->setFocusPolicy(Qt::StrongFocus);
	ui.lblConnectSettings->installEventFilter(this);

	connect(ui.lneNode,SIGNAL(textChanged(const QString &)),SLOT(onLoginOrPasswordTextChanged()));
	connect(ui.lnePassword,SIGNAL(textChanged(const QString &)),SLOT(onLoginOrPasswordTextChanged()));

	ui.cmbDomain->addItem("@rambler.ru",QString("rambler.ru"));
	ui.cmbDomain->addItem("@lenta.ru",QString("lenta.ru"));
	ui.cmbDomain->addItem("@myrambler.ru",QString("myrambler.ru"));
	ui.cmbDomain->addItem("@autorambler.ru",QString("autorambler.ru"));
	ui.cmbDomain->addItem("@ro.ru",QString("ro.ru"));
	ui.cmbDomain->addItem("@r0.ru",QString("r0.ru"));

	Action * action = new Action(domainsMenu);
	action->setText("@rambler.ru");
	action->setData(Action::DR_UserDefined + 1, QString("rambler.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	domainsMenu->addAction(action);

	action->trigger();

	action = new Action(domainsMenu);
	action->setText("@lenta.ru");
	action->setData(Action::DR_UserDefined + 1, QString("lenta.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	domainsMenu->addAction(action);

	action = new Action(domainsMenu);
	action->setText("@myrambler.ru");
	action->setData(Action::DR_UserDefined + 1, QString("myrambler.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	domainsMenu->addAction(action);

	action = new Action(domainsMenu);
	action->setText("@autorambler.ru");
	action->setData(Action::DR_UserDefined + 1, QString("autorambler.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	domainsMenu->addAction(action);

	action = new Action(domainsMenu);
	action->setText("@ro.ru");
	action->setData(Action::DR_UserDefined + 1, QString("ro.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	domainsMenu->addAction(action);

	action = new Action(domainsMenu);
	action->setText("@r0.ru");
	action->setData(Action::DR_UserDefined + 1, QString("r0.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	domainsMenu->addAction(action);

	//ui.tlbDomain->setVisible(false);
	ui.cmbDomain->setVisible(false);

	QStringList profiles;
	foreach(QString profile, FOptionsManager->profiles())
	{
		Jid streamJid = Jid::decode(profile);
		if (streamJid.isValid() && !streamJid.node().isEmpty())
		{
			if (ui.cmbDomain->findData(streamJid.pDomain())<0)
				ui.cmbDomain->insertItem(0,"@"+streamJid.pDomain(),streamJid.pDomain());
			profiles.append(streamJid.bare());
		}
	}
	ui.cmbDomain->addItem(tr("Custom domain..."));
	ui.cmbDomain->insertSeparator(ui.cmbDomain->count() - 1);
	connect(ui.cmbDomain,SIGNAL(currentIndexChanged(int)),SLOT(onDomainCurrentIntexChanged(int)));

	QCompleter *completer = new QCompleter(profiles,ui.lneNode);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	completer->setCompletionMode(QCompleter::PopupCompletion);
	//completer->setPopup(new CustomListView);
	completer->setPopup(new QListView);
	completer->popup()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	completer->popup()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	completer->popup()->setObjectName("completerPopUp");
	completer->popup()->setMouseTracking(true);
	completer->popup()->setAlternatingRowColors(true);
	completer->popup()->setItemDelegate(new CompleterDelegate(completer));
	connect(completer,SIGNAL(activated(const QString &)),SLOT(onCompleterActivated(const QString &)));
	connect(completer,SIGNAL(highlighted(const QString &)),SLOT(onCompleterHighLighted(const QString &)));
	ui.lneNode->setCompleter(completer);
	ui.lneNode->completer()->popup()->viewport()->installEventFilter(this);
	onStylePreviewReset();

	ui.lneNode->installEventFilter(this);
	ui.lneNode->completer()->popup()->installEventFilter(this);
	if (ui.lneNode->completer()->popup()->parentWidget())
		ui.lneNode->completer()->popup()->parentWidget()->installEventFilter(this);
	ui.cmbDomain->installEventFilter(this);
	ui.lnePassword->installEventFilter(this);
	ui.chbSavePassword->installEventFilter(this);
	ui.chbAutoRun->installEventFilter(this);
	ui.lblLogo->installEventFilter(this);
	ui.lblForgotPassword->installEventFilter(this);
	ui.lblRegister->installEventFilter(this);
	ui.wdtContent->installEventFilter(this);

	if (FMainWindowPlugin)
	{
		if (FMainWindowPlugin->mainWindowBorder())
		{
			FMainWindowPlugin->mainWindowBorder()->installEventFilter(this);
			FMainWindowVisible = FMainWindowPlugin->mainWindowBorder()->isVisible();
			FMainWindowPlugin->mainWindowBorder()->hide();
		}
		else
		{
			FMainWindowPlugin->mainWindow()->instance()->installEventFilter(this);
			FMainWindowVisible = FMainWindowPlugin->mainWindow()->instance()->isVisible();
			FMainWindowPlugin->mainWindow()->instance()->hide();
		}
	}

	FReconnectTimer.setSingleShot(true);
	connect(&FReconnectTimer,SIGNAL(timeout()),SLOT(onReconnectTimerTimeout()));

	ui.pbtConnect->setFocus();
	connect(ui.pbtConnect,SIGNAL(clicked()),SLOT(onConnectClicked()));

	hideXmppStreamError();
	hideConnectionError();
	setConnectEnabled(true);
	onLoginOrPasswordTextChanged();
}

LoginDialog::~LoginDialog()
{

}

void LoginDialog::loadLastProfile()
{
	Jid lastStreamJid = Jid::decode(FOptionsManager->lastActiveProfile());
	if (lastStreamJid.isValid())
	{
		ui.lneNode->setText(lastStreamJid.pNode());
		QString domain = lastStreamJid.pDomain();
		ui.cmbDomain->setCurrentIndex(ui.cmbDomain->findData(domain));
		ui.tlbDomain->setText("@"+domain);
		ui.tlbDomain->setProperty("domain", domain);
		loadCurrentProfileSettings();
	}
}

void LoginDialog::connectIfReady()
{
	if (readyToConnect())
		onConnectClicked();
}

Jid LoginDialog::currentStreamJid() const
{
	//Jid streamJid(ui.lneNode->text().trimmed(),ui.cmbDomain->itemData(ui.cmbDomain->currentIndex()).toString(),CLIENT_NAME);
	Jid streamJid(ui.lneNode->text().trimmed(), ui.tlbDomain->property("domain").toString(), CLIENT_NAME);
	return streamJid;
}

void LoginDialog::reject()
{
	if (!FAccountId.isNull())
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
		if (!account || !account->isActive() || !account->xmppStream()->isOpen())
			closeCurrentProfile();
	}
	QDialog::reject();
}

void LoginDialog::showEvent(QShowEvent *AEvent)
{
	QDialog::showEvent(AEvent);
	QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
}

void LoginDialog::keyPressEvent(QKeyEvent *AEvent)
{
	if (AEvent->key()==Qt::Key_Return || AEvent->key()==Qt::Key_Enter)
	{
		if (ui.pbtConnect->isEnabled())
			QTimer::singleShot(0,this,SLOT(onConnectClicked()));
	}
	QDialog::keyPressEvent(AEvent);
}

bool LoginDialog::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::MouseButtonPress)
	{
		if (AWatched == ui.lneNode || AWatched == ui.tlbDomain || AWatched == ui.lnePassword || AWatched == ui.chbSavePassword || AWatched == ui.chbAutoRun)
		{
			stopReconnection();
		}
		if ( AWatched != ui.lnePassword)
			BalloonTip::hideBalloon();
	}
	else if (AEvent->type() == QEvent::FocusIn)
	{
		if (AWatched == ui.lneNode)
		{
			ui.lneNode->event(AEvent);
			disconnect(ui.lneNode->completer(),0,ui.lneNode,0);
			ui.lneNode->completer()->complete();
			return true;
		}
		else if (AWatched == ui.lnePassword && isCapsLockOn())
		{
			QPoint p = ui.lnePassword->mapToGlobal(ui.lnePassword->rect().bottomLeft());
			p.setY(p.y() - ui.lnePassword->height() / 2);
			if (isActiveWindow() || (parentWidget() && parentWidget()->isActiveWindow()))
				BalloonTip::showBalloon(style()->standardIcon(QStyle::SP_MessageBoxWarning), tr("Caps Lock is ON"),
							tr("Password can be entered incorrectly because of <CapsLock> key is pressed.\nTurn off <CapsLock> before entering password."),
							p, 0, true, BalloonTip::ArrowRight);
		}
	}
	else if (AEvent->type() == QEvent::FocusOut)
	{
		if (AWatched == ui.lnePassword)
		{
			BalloonTip::hideBalloon();
		}
		else if (AWatched == ui.lneNode)
		{
			ui.lneNode->completer()->popup()->hide();
		}
	}
	else if (AEvent->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
		if (keyEvent->key() == Qt::Key_CapsLock && !isCapsLockOn())
		{
			BalloonTip::hideBalloon();
		}
	}
	else if (AEvent->type() == QEvent::Show)
	{
		if (AWatched == ui.lneNode->completer()->popup())
		{
			// TODO: adjust size to popup contents
			//ui.lneNode->completer()->popup()->setFixedWidth(ui.lneNode->completer()->popup()->sizeHint().width());
			ui.lneNode->completer()->popup()->setFixedWidth(ui.lneNode->width() * 1.2);
			ui.lneNode->completer()->popup()->move(ui.lneNode->completer()->popup()->pos().x() + 1, ui.lneNode->completer()->popup()->pos().y() + 1);
		}
		else if (AWatched == ui.lneNode->completer()->popup()->parentWidget())
		{
			//ui.lneNode->completer()->popup()->parentWidget()->setFixedWidth(ui.lneNode->width() * 1.2);
			//ui.lneNode->completer()->popup()->parentWidget()->move(ui.lneNode->completer()->popup()->parentWidget()->pos().x() + 1, ui.lneNode->completer()->popup()->parentWidget()->pos().y() + 1);
			ui.lneNode->completer()->popup()->setFixedWidth(ui.lneNode->width() * 1.2);
			ui.lneNode->completer()->popup()->move(ui.lneNode->completer()->popup()->pos().x() + 1, ui.lneNode->completer()->popup()->pos().y() + 1);
			ui.lneNode->completer()->popup()->show();
			//ui.lneNode->completer()->popup()->parentWidget()->adjustSize();
			//ui.lneNode->completer()->popup()->parentWidget()->layout()->update();
		}
		else if (FMainWindowPlugin && (AWatched == FMainWindowPlugin->mainWindow()->instance() || AWatched == FMainWindowPlugin->mainWindowBorder()))
		{
			FMainWindowVisible = true;
			if (AWatched == FMainWindowPlugin->mainWindow()->instance())
			{
#ifdef Q_WS_WIN
				if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
					QTimer::singleShot(0,FMainWindowPlugin->mainWindow()->instance(), SLOT(hide()));
				else
#endif
					QTimer::singleShot(0,FMainWindowPlugin->mainWindow()->instance(), SLOT(close()));
			}
			else
#ifdef Q_WS_WIN
				if (QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7)
					QTimer::singleShot(0,FMainWindowPlugin->mainWindowBorder(), SLOT(hide()));
				else
#endif
				QTimer::singleShot(0,FMainWindowPlugin->mainWindowBorder(), SLOT(closeWidget()));
		}
	}
	if (AWatched == ui.lblConnectSettings)
	{
		if (AEvent->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent * mouseEvent = (QMouseEvent*)AEvent;
			if (mouseEvent->button() == Qt::LeftButton)
			{
				hideConnectionError();
				hideXmppStreamError();
				showConnectionSettings();
			}
		}
	}
	if (ui.lneNode->completer()->popup() && (AWatched == ui.lneNode->completer()->popup()->viewport()) && (AEvent->type() == QEvent::MouseMove))
	{
		QMouseEvent * mouseEvent = (QMouseEvent*)AEvent;
		QListView * view = qobject_cast<QListView*>(ui.lneNode->completer()->popup());
		if (view)
		{
			QModelIndex index = view->indexAt(mouseEvent->pos());
			if (index.isValid() && index != view->currentIndex())
				view->setCurrentIndex(index);
		}
	}
	if (AWatched == parentWidget())
	{
		if (AEvent->type() == QEvent::Move)
			BalloonTip::hideBalloon();
	}

	return QDialog::eventFilter(AWatched, AEvent);
}

void LoginDialog::moveEvent(QMoveEvent * evt)
{
	BalloonTip::hideBalloon();
	QDialog::moveEvent(evt);
}

void LoginDialog::mousePressEvent(QMouseEvent * event)
{
	BalloonTip::hideBalloon();
	QDialog::mousePressEvent(event);
}

void LoginDialog::initialize(IPluginManager *APluginManager)
{
	FOptionsManager = NULL;
	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	FAccountManager = NULL;
	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
	}

	FStatusChanger = NULL;
	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}

	FMainWindowPlugin = NULL;
	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	FConnectionManager = NULL;
	plugin = APluginManager->pluginInterface("IConnectionManager").value(0,NULL);
	if (plugin)
	{
		FConnectionManager = qobject_cast<IConnectionManager *>(plugin->instance());
	}

	FTrayManager = NULL;
	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
				SLOT(onTrayNotifyActivated(int,QSystemTrayIcon::ActivationReason)));
		}
	}

	FNotifications = NULL;
	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationAppend(int, INotification &)),SLOT(onNotificationAppend(int, INotification &)));
			connect(FNotifications->instance(),SIGNAL(notificationAppended(int, const INotification &)),SLOT(onNotificationAppended(int, const INotification &)));
		}
	}
}

bool LoginDialog::isCapsLockOn() const
{
#ifdef Q_WS_WIN
	return GetKeyState(VK_CAPITAL) == 1;
#elif defined Q_WS_X11
	Display * d = XOpenDisplay((char*)0);
	bool caps_state = false;
	if (d)
	{
		unsigned n;
		XkbGetIndicatorState(d, XkbUseCoreKbd, &n);
		caps_state = (n & 0x01) == 1;
	}
	return caps_state;
#endif
	return false;
}

void LoginDialog::closeCurrentProfile()
{
	if (!FNewProfile)
		FOptionsManager->setCurrentProfile(QString::null,QString::null);
	else if (FOptionsManager->isOpened())
		FOptionsManager->removeProfile(FOptionsManager->currentProfile());
}

bool LoginDialog::tryNextConnectionSettings()
{
	if (FNewProfile && FFirstConnect)
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
		IConnection *connection = account!=NULL && account->isActive() ? account->xmppStream()->connection() : NULL;
		if (connection)
		{
			IDefaultConnection *defConnection = qobject_cast<IDefaultConnection *>(connection->instance());
			if (defConnection)
			{
				FConnectionSettings++;
				if (FConnectionSettings == CS_IE_PROXY)
				{
					if (FConnectionManager && FConnectionManager->proxyList().contains(IEXPLORER_PROXY_REF_UUID))
					{
						IConnectionProxy proxy = FConnectionManager->proxyById(IEXPLORER_PROXY_REF_UUID);
						defConnection->setProxy(proxy.proxy);
						return true;
					}
					return tryNextConnectionSettings();
				}
				else if (FConnectionSettings == CS_FF_PROXY)
				{
					if (FConnectionManager && FConnectionManager->proxyList().contains(FIREFOX_PROXY_REF_UUID))
					{
						IConnectionProxy proxy = FConnectionManager->proxyById(FIREFOX_PROXY_REF_UUID);
						defConnection->setProxy(proxy.proxy);
						return true;
					}
					return tryNextConnectionSettings();
				}
				else
				{
					FConnectionSettings = CS_DEFAULT;
					connection->ownerPlugin()->loadConnectionSettings(connection,account->optionsNode().node("connection",connection->ownerPlugin()->pluginId()));
				}
			}
		}
	}
	return false;
}

void LoginDialog::setConnectEnabled(bool AEnabled)
{
	if (!AEnabled)
	{
		FReconnectTimer.stop();
		if (!ui.lblReconnect->text().isEmpty())
			ui.lblReconnect->setText(tr("Reconnecting..."));
		BalloonTip::hideBalloon();
		QTimer::singleShot(3000,this,SLOT(onShowConnectingAnimation()));
	}
	else
	{
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->removeAutoIcon(ui.pbtConnect);
		ui.pbtConnect->setIcon(QIcon());
	}

	ui.lneNode->setEnabled(AEnabled);
	ui.cmbDomain->setEnabled(AEnabled);
	ui.tlbDomain->setEnabled(AEnabled);
	ui.lnePassword->setEnabled(AEnabled);
	ui.chbSavePassword->setEnabled(AEnabled);
	ui.chbAutoRun->setEnabled(AEnabled);

	if (AEnabled)
		onLoginOrPasswordTextChanged();
	else
	{
		ui.pbtConnect->setEnabled(AEnabled);
		ui.pbtConnect->setProperty("connecting", true);
		StyleStorage::updateStyle(this);
	}
	ui.pbtConnect->setText(AEnabled ? tr("Enter") : tr("Connecting..."));
}

void LoginDialog::stopReconnection()
{
	if (FReconnectTimer.isActive())
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
		if (FStatusChanger && account && account->isActive())
			FStatusChanger->setStreamStatus(account->xmppStream()->streamJid(),STATUS_OFFLINE);

		FReconnectTimer.stop();
		ui.lblReconnect->setText(QString::null);
	}
}

void LoginDialog::showConnectionSettings()
{
	stopReconnection();
	IOptionsHolder *holder = FConnectionManager!=NULL ? qobject_cast<IOptionsHolder *>(FConnectionManager->instance()) : NULL;
	if (holder)
	{
		QDialog *dialog = new QDialog(this);
		dialog->setAttribute(Qt::WA_DeleteOnClose,true);
		dialog->setLayout(new QVBoxLayout);
		dialog->layout()->setContentsMargins(18, 4, 8, 7);
		dialog->setFixedWidth(330);

		StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(dialog, STS_OPTIONS_CONNECTION_SETTINGS);

		CustomBorderContainer * dialogBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(dialog, CBS_DIALOG);
		if (dialogBorder)
		{
			dialogBorder->setMinimizeButtonVisible(false);
			dialogBorder->setMaximizeButtonVisible(false);
			dialogBorder->setWindowTitle(windowTitle());
			dialogBorder->setResizable(false);
			connect(this, SIGNAL(accepted()), dialogBorder, SLOT(closeWidget()));
			connect(this, SIGNAL(rejected()), dialogBorder, SLOT(closeWidget()));
			connect(dialogBorder, SIGNAL(closeClicked()), dialog, SLOT(reject()));
			dialog->setAttribute(Qt::WA_DeleteOnClose, true);
		}

		// extra layout for contents
		QVBoxLayout * settingsLayout = new QVBoxLayout;
		settingsLayout->setContentsMargins(0, 0, 14, 8);

		foreach(IOptionsWidget *widget, holder->optionsWidgets(OPN_CONNECTION, dialog))
		{
			settingsLayout->addWidget(widget->instance());
			connect(dialog,SIGNAL(accepted()),widget->instance(),SLOT(apply()));
		}

		dialog->layout()->addItem(settingsLayout);

		QDialogButtonBox *buttons = new QDialogButtonBox(dialog);
		buttons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
		buttons->button(QDialogButtonBox::Ok)->setAutoDefault(false);
		buttons->button(QDialogButtonBox::Ok)->setDefault(true);
		buttons->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
		dialog->layout()->addWidget(buttons);
		connect(buttons,SIGNAL(accepted()),dialog,SLOT(accept()));
		connect(buttons,SIGNAL(rejected()),dialog,SLOT(reject()));

		dialog->setWindowTitle(tr("Connection settings"));

		WidgetManager::showActivateRaiseWindow(dialogBorder ? (QWidget*)dialogBorder : (QWidget*)dialog);
	}
}

void LoginDialog::hideConnectionError()
{
	BalloonTip::hideBalloon();
	ui.lblConnectError->setVisible(false);
	ui.lblReconnect->setVisible(false);
	ui.lblConnectSettings->setVisible(false);
}

void LoginDialog::showConnectionError(const QString &ACaption, const QString &AError)
{
	Log(QString("[LoginDialog connection error] %1").arg(AError));
	hideXmppStreamError();

	QString message = ACaption;
	message += message.isEmpty() || AError.isEmpty() ? AError : "<br>" + AError;
	ui.lblConnectError->setText(message);

	ui.lblConnectSettings->setText(QString("<a href='ramblercontacts.connection.settings'>%1</a>").arg(tr("Connection settings")));

	int tries = FReconnectTimer.property("tries").toInt();
	if (tries > 0)
	{
		FReconnectTimer.start(0);
		FReconnectTimer.setProperty("ticks",10);
		FReconnectTimer.setProperty("tries",tries-1);
		ui.lblReconnect->setText(tr("Reconnect after <b>%1</b> secs").arg(10));
	}
	else
		ui.lblReconnect->setText(tr("Reconnection failed"));

	ui.lblConnectError->setVisible(true);
	ui.lblReconnect->setVisible(true);
	ui.lblConnectSettings->setVisible(true);
	ui.chbShowPassword->setVisible(false);
	QPoint p = ui.pbtConnect->mapToGlobal(ui.pbtConnect->rect().topLeft());
	p.setY(p.y() + ui.pbtConnect->height() / 2);
	if (isActiveWindow() || (parentWidget() && parentWidget()->isActiveWindow()))
		BalloonTip::showBalloon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_OPTIONS_ERROR_ALERT), FConnectionErrorWidget, p, 0, true, BalloonTip::ArrowRight);
}

void LoginDialog::hideXmppStreamError()
{
	ui.lneNode->setProperty("error", false);
	ui.lnePassword->setProperty("error", false);
	ui.cmbDomain->setProperty("error", false);
	ui.tlbDomain->setProperty("error", false);
	//	ui.frmDomain->setProperty("error", false);
	StyleStorage::updateStyle(this);
	ui.lblXmppError->setVisible(false);
	BalloonTip::hideBalloon();
}

void LoginDialog::showXmppStreamError(const QString &ACaption, const QString &AError, const QString &AHint, bool showPasswordEnabled)
{
	Log(QString("[LoginDialog stream error] %1 (%2)").arg(AError, AHint));
	hideConnectionError();

	QString message = ACaption;
	message += message.isEmpty() || AError.isEmpty() ? AError : "<br>" + AError;
	message += message.isEmpty() || AHint.isEmpty() ? AHint : "<br>" + AHint;
	ui.lblXmppError->setText(message);

	if (FNewProfile)
	{
		ui.lneNode->setProperty("error", true);
		//		ui.frmDomain->setProperty("error", true);
		ui.cmbDomain->setProperty("error", true);
		ui.tlbDomain->setProperty("error", true);
	}
	ui.lnePassword->setProperty("error", true);
	StyleStorage::updateStyle(this);
	ui.lblXmppError->setVisible(true);
	ui.chbShowPassword->setVisible(showPasswordEnabled);
	QPoint p;
	if (FNewProfile)
	{
		//p = ui.cmbDomain->mapToGlobal(ui.cmbDomain->rect().topRight());
		//p.setY(p.y() + ui.cmbDomain->height() / 2);
		p = ui.tlbDomain->mapToGlobal(ui.tlbDomain->rect().topRight());
		p.setY(p.y() + ui.tlbDomain->height() / 2);
	}
	else
	{
		p = ui.lnePassword->mapToGlobal(ui.lnePassword->rect().topRight());
		p.setY(p.y() + ui.lnePassword->height() / 2);
	}
	if (isActiveWindow() || (parentWidget() && parentWidget()->isActiveWindow()))
		BalloonTip::showBalloon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_OPTIONS_ERROR_ALERT), FConnectionErrorWidget, p, 0, true, BalloonTip::ArrowLeft);
}

void LoginDialog::onConnectClicked()
{
	if (ui.pbtConnect->isEnabled())
	{
		hideConnectionError();
		hideXmppStreamError();
		ui.lneNode->completer()->popup()->hide();
		bool connecting = false;
		setConnectEnabled(false);
		QApplication::processEvents();

		Jid streamJid = currentStreamJid();
		QString profile = Jid::encode(streamJid.pBare());
		if (FOptionsManager->currentProfile() != profile)
		{
			FFirstConnect = true;
			closeCurrentProfile();
			FReconnectTimer.setProperty("tries",20);
		}

		if (streamJid.isValid() && !streamJid.node().isEmpty())
		{
			if (!FOptionsManager->isOpened())
				FNewProfile = !FOptionsManager->profiles().contains(profile);

			if (!FNewProfile || FOptionsManager->isOpened() || FOptionsManager->addProfile(profile,QString::null))
			{
				if (FOptionsManager->setCurrentProfile(profile,QString::null))
				{
					IAccount *account = FAccountManager!=NULL ? FAccountManager->accounts().value(0, NULL) : NULL;
					if (FAccountManager && !account)
						account = FAccountManager->appendAccount(QUuid::createUuid());

					if (account)
					{
						account->setName(streamJid.domain());
						account->setStreamJid(streamJid);
						account->setActive(true);
						if (FStatusChanger && account->isActive())
						{
							connecting = true;
							FAccountId = account->accountId();
							disconnect(account->xmppStream()->instance(),0,this,0);
							connect(account->xmppStream()->instance(),SIGNAL(opened()),SLOT(onXmppStreamOpened()));
							connect(account->xmppStream()->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()));

							account->setPassword(ui.chbSavePassword->isChecked() ? ui.lnePassword->text() : QString::null);
							account->xmppStream()->setPassword(ui.lnePassword->text());

							FStatusChanger->setStreamStatus(account->xmppStream()->streamJid(), STATUS_MAIN_ID);

							int mainShow = FStatusChanger->statusItemShow(FStatusChanger->mainStatus());
							if (mainShow==IPresence::Offline || mainShow==IPresence::Error)
								FStatusChanger->setMainStatus(STATUS_ONLINE);
						}
						else
							showXmppStreamError(tr("Unable to activate account"), QString::null, tr("Internal error, contact support"));
					}
					else
						showXmppStreamError(tr("Unable to create account"), QString::null, tr("Internal error, contact support"));
				}
				else
					showXmppStreamError(tr("Unable to open profile"), QString::null, tr("This profile is already opened by another Contacts instance"), false);
			}
			else
				showXmppStreamError(tr("Unable to create profile"), QString::null, tr("Check your system permissions to create folders"));
		}
		else
			showXmppStreamError(tr("Invalid login"), QString::null ,tr("Check your user name and domain"));

		setConnectEnabled(!connecting);
	}
	QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
}

void LoginDialog::onXmppStreamOpened()
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
	if (account && FConnectionSettings!=CS_DEFAULT)
	{
		OptionsNode coptions = account->optionsNode().node("connection",account->xmppStream()->connection()->ownerPlugin()->pluginId());
		if (FConnectionSettings == CS_IE_PROXY)
			coptions.setValue(IEXPLORER_PROXY_REF_UUID,"proxy");
		else if (FConnectionSettings == CS_FF_PROXY)
			coptions.setValue(FIREFOX_PROXY_REF_UUID,"proxy");
	}

	Options::node(OPV_MISC_AUTOSTART).setValue(ui.chbAutoRun->isChecked());

	if (FMainWindowPlugin)
	{
		if (FMainWindowPlugin->mainWindowBorder())
		{
			FMainWindowPlugin->mainWindowBorder()->removeEventFilter(this);
			if (FMainWindowVisible)
				FMainWindowPlugin->mainWindowBorder()->show();
		}
		else
		{
			FMainWindowPlugin->mainWindow()->instance()->removeEventFilter(this);
			if (FMainWindowVisible)
				FMainWindowPlugin->mainWindow()->instance()->show();
		}
	}

	saveCurrentProfileSettings();
	accept();
}

void LoginDialog::onXmppStreamClosed()
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
	if (account && account->xmppStream()->connection() == NULL)
	{
		showConnectionError(tr("Unable to set connection"), tr("Internal error, contact support"));
		stopReconnection();
	}
	else if (account && !account->xmppStream()->connection()->errorString().isEmpty())
	{
		if (tryNextConnectionSettings())
		{
			QTimer::singleShot(0,this,SLOT(onConnectClicked()));
			return;
		}
		else
			showConnectionError(tr("Unable to connect to server"),account->xmppStream()->connection()->errorString());
	}
	else if (account)
	{
		showXmppStreamError(FNewProfile ? tr("Login and password mismatch") : tr("Wrong password"), QString::null/*account->xmppStream()->errorString()*/,
				    tr("Check keyboard layout"));
	}

	FFirstConnect = false;
	setConnectEnabled(true);
	QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
}

void LoginDialog::onReconnectTimerTimeout()
{
	int ticks = FReconnectTimer.property("ticks").toInt();
	if (ticks > 0)
	{
		ui.lblReconnect->setText(tr("Reconnect after <b>%1</b> secs").arg(ticks));
		FReconnectTimer.setProperty("ticks",ticks-1);
		FReconnectTimer.start(1000);
	}
	else if (ticks == 0)
	{
		onConnectClicked();
	}
}

void LoginDialog::onCompleterHighLighted(const QString &AText)
{
	Jid streamJid = AText;
	ui.lneNode->setText(streamJid.node());
	ui.cmbDomain->setCurrentIndex(ui.cmbDomain->findData(streamJid.pDomain()));
	ui.tlbDomain->setText("@"+streamJid.pDomain());
	ui.tlbDomain->setProperty("domain", streamJid.pDomain());
}

void LoginDialog::onCompleterActivated(const QString &AText)
{
	onCompleterHighLighted(AText);
	loadCurrentProfileSettings();
	hideXmppStreamError();
	hideConnectionError();
}

void LoginDialog::onDomainCurrentIntexChanged(int AIndex)
{
	if (ui.cmbDomain->itemData(AIndex).toString().isEmpty())
	{
		CustomInputDialog *dialog = new CustomInputDialog(CustomInputDialog::String);
		dialog->setCaptionText(QString::null);
		dialog->setWindowTitle(tr("Add custom domain"));
		dialog->setInfoText(tr("Enter address of custom domain\nwhich is linked to Rambler"));
		dialog->setAcceptButtonText(tr("Add"));
		dialog->setRejectButtonText(tr("Cancel"));
		dialog->setDeleteOnClose(true);
		dialog->setDescriptionText(QString("<a href='http://partners.mail.rambler.ru'>%1</a>").arg(tr("How to link your domain?")));
		connect(dialog, SIGNAL(stringAccepted(const QString &)), SLOT(onNewDomainSelected(const QString &)));
		connect(dialog, SIGNAL(rejected()), SLOT(onNewDomainRejected()));
		connect(dialog, SIGNAL(linkActivated(const QString &)), SLOT(onLabelLinkActivated(const QString &)));
		dialog->show();
	}
	else
		domainPrevIndex = AIndex;
}

void LoginDialog::onDomainActionTriggered()
{
	Action * action = qobject_cast<Action*>(sender());
	if (action)
	{
		QString domain = action->data(Action::DR_UserDefined + 1).toString();
		ui.tlbDomain->setText(action->text());
		ui.tlbDomain->setProperty("domain", domain);
	}
}

void LoginDialog::onNewDomainSelected(const QString & newDomain)
{
	if (!newDomain.isEmpty())
	{
		Jid domain = newDomain;
		int index = ui.cmbDomain->findData(domain.pDomain());
		if (index < 0)
		{
			index = 0;
			ui.cmbDomain->blockSignals(true);
			ui.cmbDomain->insertItem(0,"@"+domain.pDomain(),domain.pDomain());
			ui.cmbDomain->blockSignals(false);
		}
		ui.cmbDomain->setCurrentIndex(index);
	}
	else
	{
		onNewDomainRejected();
	}
}

void LoginDialog::onNewDomainRejected()
{
	ui.cmbDomain->setCurrentIndex(domainPrevIndex);
}

void LoginDialog::onLabelLinkActivated(const QString &ALink)
{
	if (ALink == "ramblercontacts.connection.settings")
		showConnectionSettings();
	else
		QDesktopServices::openUrl(ALink);
}

void LoginDialog::saveCurrentProfileSettings()
{
	Jid streamJid = currentStreamJid();
	QString profile = Jid::encode(streamJid.pBare());
	if (FOptionsManager->profiles().contains(profile))
	{
		QFile login(QDir(FOptionsManager->profilePath(profile)).absoluteFilePath(FILE_LOGIN));
		if (login.open(QFile::WriteOnly|QFile::Truncate))
		{
			QDomDocument doc;
			doc.appendChild(doc.createElement("login-settings"));

			QDomElement passElem = doc.documentElement().appendChild(doc.createElement("password")).toElement();
			if (ui.chbSavePassword->isChecked())
			{
				passElem.setAttribute("save","true");
				passElem.appendChild(doc.createTextNode(QString::fromLatin1(Options::encrypt(ui.lnePassword->text(),FOptionsManager->profileKey(profile,QString::null)))));
			}
			else
			{
				passElem.setAttribute("save","false");
			}

			QDomElement autoElem = doc.documentElement().appendChild(doc.createElement("auto-run")).toElement();
			autoElem.appendChild(doc.createTextNode(QVariant(ui.chbAutoRun->isChecked()).toString()));

			login.write(doc.toByteArray());
			login.close();
		}
	}
}

void LoginDialog::loadCurrentProfileSettings()
{
	Jid streamJid = currentStreamJid();
	QString profile = Jid::encode(streamJid.pBare());
	if (FOptionsManager->profiles().contains(profile))
	{
		QDomDocument doc;
		QFile login(QDir(FOptionsManager->profilePath(profile)).absoluteFilePath(FILE_LOGIN));
		if (login.open(QFile::ReadOnly) && doc.setContent(&login))
		{
			QDomElement pasElem = doc.documentElement().firstChildElement("password");
			if (!pasElem.isNull() && QVariant(pasElem.attribute("save")).toBool())
			{
				FSavedPasswordCleared = false;
				ui.chbSavePassword->setChecked(true);
				ui.chbShowPassword->setChecked(false);
				ui.lnePassword->setEchoMode(QLineEdit::Password);
				ui.lnePassword->setText(Options::decrypt(pasElem.text().toLatin1(),FOptionsManager->profileKey(profile,QString::null)).toString());
			}
			else
			{
				FSavedPasswordCleared = true;
				ui.chbSavePassword->setChecked(false);
				ui.lnePassword->setText(QString::null);
			}

			QDomElement autoElem = doc.documentElement().firstChildElement("auto-run");
			if (!autoElem.isNull())
			{
				ui.chbAutoRun->setChecked(QVariant(autoElem.text()).toBool());
			}
			else
			{
				ui.chbAutoRun->setChecked(false);
			}
		}
		login.close();
	}
}

bool LoginDialog::readyToConnect() const
{
	return ui.chbSavePassword->isChecked() && !ui.lnePassword->text().isEmpty();
}

void LoginDialog::onLoginOrPasswordTextChanged()
{
	ui.pbtConnect->setEnabled(!ui.lneNode->text().isEmpty() && !ui.lnePassword->text().isEmpty());
	ui.pbtConnect->setProperty("connecting", false);
	StyleStorage::updateStyle(this);
}

void LoginDialog::onShowConnectingAnimation()
{
	if (!ui.pbtConnect->isEnabled())
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.pbtConnect,MNI_OPTIONS_LOGIN_ANIMATION);
}

void LoginDialog::onAdjustDialogSize()
{
	//resize(minimumSizeHint());
}

void LoginDialog::onNotificationAppend(int ANotifyId, INotification &ANotification)
{
	Q_UNUSED(ANotifyId);
	ANotification.kinds = 0;
}

void LoginDialog::onNotificationAppended(int ANotifyId, const INotification &ANotification)
{
	Q_UNUSED(ANotification);
	FNotifications->removeNotification(ANotifyId);
}

void LoginDialog::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	if (ANotifyId<0 && AReason==QSystemTrayIcon::DoubleClick)
	{
		WidgetManager::showActivateRaiseWindow(this);
	}
}

void LoginDialog::onShowPasswordToggled(int state)
{
	if (state == Qt::Checked)
	{
		if (!FSavedPasswordCleared)
			ui.lnePassword->clear();
		FSavedPasswordCleared = true;
		ui.lnePassword->setEchoMode(QLineEdit::Normal);
	}
	else
		ui.lnePassword->setEchoMode(QLineEdit::Password);
}

void LoginDialog::onStylePreviewReset()
{
	if (ui.lneNode->completer())
		ui.lneNode->completer()->popup()->setStyleSheet(styleSheet());
}

#include "logindialog.moc"
