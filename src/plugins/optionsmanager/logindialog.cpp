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
#include <definitions/customborder.h>
#include <definitions/resources.h>
#include <definitions/graphicseffects.h>
#include <definitions/menuicons.h>
#include <utils/log.h>
#include <utils/iconstorage.h>
#include <utils/customlistview.h>
#include <utils/custominputdialog.h>
#include <utils/customborderstorage.h>
#include <utils/graphicseffectsstorage.h>

#ifdef Q_WS_MAC
# include <utils/macwidgets.h>
# include <Carbon/Carbon.h>
#endif

#ifdef Q_WS_WIN32
#	include <windows.h>
#elif defined Q_WS_X11
#	include <X11/XKBlib.h>
#	undef KeyPress
#	undef FocusIn
#	undef FocusOut
#endif

#define FILE_LOGIN            "login.xml"
#define ABORT_TIMEOUT         2000

enum ConnectionSettings {
	CS_DEFAULT,
	CS_IE_PROXY,
	CS_FF_PROXY,
	CS_COUNT
};

enum ActiveErrorType {
	NoActiveError,
	ActiveXmppError,
	ActiveConnectionError
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

class DomainComboDelegate :
	public QStyledItemDelegate
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
			QImage separator = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_COMMON_MENUSEPARATOR);
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
			QImage separator = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getImage(MNI_COMMON_MENUSEPARATOR);
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
	//setWindowModality(Qt::WindowModal); // what for?
	setAttribute(Qt::WA_DeleteOnClose, true);
#ifdef Q_WS_MAC
	setWindowGrowButtonEnabled(this->window(), false);
	ui.frmLogin->layout()->setSpacing(6);
	QVBoxLayout * lt = qobject_cast<QVBoxLayout*>(ui.wdtContent->layout());
	if (lt)
	{
		lt->insertStretch(lt->indexOf(ui.chbAutoRun));
	}
	ui.chbAutoRun->setVisible(false);
#endif

	ui.lneNode->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lnePassword->setAttribute(Qt::WA_MacShowFocusRect, false);

	FActiveErrorType = NoActiveError;
	FDomainPrevIndex = 0;
	FNewProfile = true;
	FSavedPasswordCleared = false;
	FConnectionSettings = CS_DEFAULT;

	FConnectionErrorWidget = new QWidget;
	FConnectionErrorWidget->setObjectName("connectionErrorWidget");
	QVBoxLayout *vlayout = new QVBoxLayout;
	vlayout->setSpacing(4);
	vlayout->setContentsMargins(0, 0, 0, 0);
	vlayout->addWidget(ui.lblConnectError);
	//vlayout->addWidget(ui.lblConnectSettings);
	vlayout->addWidget(ui.lblXmppError);
	vlayout->addWidget(ui.lblReconnect);
	vlayout->addWidget(ui.chbShowPassword);
	FConnectionErrorWidget->setLayout(vlayout);

	ui.lneNode->setProperty("error", false);
	ui.lnePassword->setProperty("error", false);
	ui.cmbDomain->setProperty("error", false);
	ui.tlbDomain->setProperty("error", false);

	FDomainsMenu = new Menu(this);
	FDomainsMenu->setObjectName("domainsMenu");
	ui.tlbDomain->setMenu(FDomainsMenu);

	ui.cmbDomain->setView(new QListView(ui.cmbDomain));
	ui.cmbDomain->view()->setItemDelegate(new DomainComboDelegate(ui.cmbDomain->view(), ui.cmbDomain));
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_OPTIONS_LOGINDIALOG);
	connect(StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS), SIGNAL(stylePreviewReset()), SLOT(onStylePreviewReset()));
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);
	FConnectionErrorWidget->setStyleSheet(styleSheet());

	initialize(APluginManager);
	FOptionsManager->setCurrentProfile(QString::null,QString::null);

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.lblLogo,MNI_OPTIONS_LOGIN_LOGO,0,0,"pixmap");
	ui.lblLogo->setFixedHeight(43);

	int fontSize = 9;
#ifdef Q_WS_MAC
	fontSize = 12;
#endif
	ui.lblRegister->setText(tr("Enter your Rambler login and password or %1.")
		.arg("<a href='http://id.rambler.ru/script/newuser.cgi'><span style=' font-size:%1pt; text-decoration: underline; color:#ffffff;'>%2</span></a>")
		.arg(fontSize)
		.arg(tr("register")));
	ui.lblForgotPassword->setText(QString("<a href='http://id.rambler.ru/script/reminder.cgi'><span style='font-size:%1pt; text-decoration: underline; color:#acacac;'>%2</span></a>")
		.arg(fontSize)
		.arg(tr("Forgot your password?")));
	ui.lblConnectSettings->setText(QString("<a href='ramblercontacts.connection.settings'><span style='font-size:%1pt; text-decoration: underline; color:#acacac;'>%2</span></a>")
		.arg(fontSize)
		.arg(tr("Connection settings")));

	//ui.lblConnectSettings->setFocusPolicy(Qt::StrongFocus);
	ui.lblConnectSettings->installEventFilter(this);

	connect(ui.lblRegister,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblForgotPassword,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));
	connect(ui.lblConnectSettings,SIGNAL(linkActivated(const QString &)),SLOT(onLabelLinkActivated(const QString &)));

	connect(ui.chbShowPassword, SIGNAL(stateChanged(int)), SLOT(onShowPasswordToggled(int)));
	connect(ui.lneNode,SIGNAL(textChanged(const QString &)),SLOT(onLoginOrPasswordTextChanged()));
	connect(ui.lnePassword,SIGNAL(textChanged(const QString &)),SLOT(onLoginOrPasswordTextChanged()));

	ui.cmbDomain->addItem("@rambler.ru",QString("rambler.ru"));
	ui.cmbDomain->addItem("@lenta.ru",QString("lenta.ru"));
	ui.cmbDomain->addItem("@myrambler.ru",QString("myrambler.ru"));
	ui.cmbDomain->addItem("@autorambler.ru",QString("autorambler.ru"));
	ui.cmbDomain->addItem("@ro.ru",QString("ro.ru"));
	ui.cmbDomain->addItem("@r0.ru",QString("r0.ru"));
	ui.cmbDomain->setSizeAdjustPolicy(QComboBox::AdjustToContents);

	Action *action = new Action(FDomainsMenu);
	action->setText("@rambler.ru");
	action->setData(Action::DR_UserDefined + 1, QString("rambler.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	FDomainsMenu->addAction(action);
	action->trigger();

	action = new Action(FDomainsMenu);
	action->setText("@lenta.ru");
	action->setData(Action::DR_UserDefined + 1, QString("lenta.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	FDomainsMenu->addAction(action);

	action = new Action(FDomainsMenu);
	action->setText("@myrambler.ru");
	action->setData(Action::DR_UserDefined + 1, QString("myrambler.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	FDomainsMenu->addAction(action);

	action = new Action(FDomainsMenu);
	action->setText("@autorambler.ru");
	action->setData(Action::DR_UserDefined + 1, QString("autorambler.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	FDomainsMenu->addAction(action);

	action = new Action(FDomainsMenu);
	action->setText("@ro.ru");
	action->setData(Action::DR_UserDefined + 1, QString("ro.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	FDomainsMenu->addAction(action);

	action = new Action(FDomainsMenu);
	action->setText("@r0.ru");
	action->setData(Action::DR_UserDefined + 1, QString("r0.ru"));
	connect(action, SIGNAL(triggered()), SLOT(onDomainActionTriggered()));
	FDomainsMenu->addAction(action);

#ifndef DEBUG_ENABLED
	ui.cmbDomain->setVisible(false);
#else
	ui.tlbDomain->setVisible(false);
#endif

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
	//connect(completer,SIGNAL(highlighted(const QString &)),SLOT(onCompleterHighLighted(const QString &)));
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

	FAbortTimer.setSingleShot(true);
	connect(&FAbortTimer,SIGNAL(timeout()),SLOT(onAbortTimerTimeout()));

	ui.pbtConnect->setFocus();
	connect(ui.pbtConnect,SIGNAL(clicked()),SLOT(onConnectClicked()));

	hideXmppStreamError();
	hideConnectionError();
	setConnectEnabled(true);
	onLoginOrPasswordTextChanged();

	LogDetail(QString("[LoginDialog] Login dialog created"));
}

LoginDialog::~LoginDialog()
{
	delete FConnectionErrorWidget;
	LogDetail(QString("[LoginDialog] Login dialog destroyed"));
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
#ifndef DEBUG_ENABLED
	Jid streamJid(ui.lneNode->text().trimmed(), ui.tlbDomain->property("domain").toString(), CLIENT_NAME);
#else
	Jid streamJid(ui.lneNode->text().trimmed(),ui.cmbDomain->itemData(ui.cmbDomain->currentIndex()).toString(),CLIENT_NAME);
#endif
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

bool LoginDialog::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::LayoutRequest)
	{
		QTimer::singleShot(0,this,SLOT(onAdjustDialogSize()));
	}
	else if (AEvent->type() == QEvent::WindowActivate)
	{
		showErrorBalloon();
	}
	return QDialog::event(AEvent);
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
	}
	else if (AEvent->type() == QEvent::MouseMove)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(AEvent);
		if (ui.lneNode->completer()->popup() && (AWatched == ui.lneNode->completer()->popup()->viewport()))
		{
			QListView *view = qobject_cast<QListView*>(ui.lneNode->completer()->popup());
			if (view)
			{
				QModelIndex index = view->indexAt(mouseEvent->pos());
				if (index.isValid() && index != view->currentIndex())
					view->setCurrentIndex(index);
			}
		}
	}
	else if (AEvent->type() == QEvent::FocusIn)
	{
		QFocusEvent *focusEvent = static_cast<QFocusEvent *>(AEvent);
		if (AWatched==ui.lneNode && focusEvent->reason()==Qt::MouseFocusReason)
		{
			ui.lneNode->event(AEvent);
			ui.lneNode->completer()->complete();
			return true;
		}
		else if (AWatched==ui.lnePassword && isCapsLockOn())
		{
			QPoint p = ui.lnePassword->mapToGlobal(ui.lnePassword->rect().bottomLeft());
			p.setY(p.y() - ui.lnePassword->height() / 2);
			showCapsLockBalloon(p);
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
		if (AWatched == ui.lnePassword)
		{
			if (isCapsLockOn() && !BalloonTip::isBalloonVisible())
			{
				QPoint p = ui.lnePassword->mapToGlobal(ui.lnePassword->rect().bottomLeft());
				p.setY(p.y() - ui.lnePassword->height() / 2);
				showCapsLockBalloon(p);
			}
			else if (!isCapsLockOn())
			{
				BalloonTip::hideBalloon();
			}
		}
		else if (AWatched==ui.lneNode && keyEvent->key()==Qt::Key_Down)
		{
			if (ui.lneNode->completer()->popup() && !ui.lneNode->completer()->popup()->isVisible())
				ui.lneNode->completer()->complete();
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

	if (ui.lneNode->completer())
		disconnect(ui.lneNode->completer(),NULL,ui.lneNode,NULL);

	return QDialog::eventFilter(AWatched, AEvent);
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
#elif defined(Q_WS_X11)
	Display * d = XOpenDisplay((char*)0);
	bool caps_state = false;
	if (d)
	{
		unsigned n;
		XkbGetIndicatorState(d, XkbUseCoreKbd, &n);
		caps_state = (n & 0x01) == 1;
	}
	return caps_state;
#elif defined(Q_WS_MAC)
	UInt32 km = GetCurrentKeyModifiers();
	return (km & 0x400);
#endif
	return false;
}

void LoginDialog::showCapsLockBalloon(const QPoint &APoint)
{
	BalloonTip::showBalloon(style()->standardIcon(QStyle::SP_MessageBoxWarning), tr("Caps Lock is ON"),
				tr("Password can be entered incorrectly because of <CapsLock> key is pressed.\nTurn off <CapsLock> before entering password."),
				APoint, 0, true, BalloonTip::ArrowRight, parentWidget() ? parentWidget() : this);
}

void LoginDialog::showErrorBalloon()
{
	if (FActiveErrorType == ActiveXmppError)
	{
		QPoint point;
		if (FNewProfile)
		{
#ifndef DEBUG_ENABLED
			point = ui.tlbDomain->mapToGlobal(ui.tlbDomain->rect().topRight());
			point.setY(point.y() + ui.tlbDomain->height() / 2);
#else
			point = ui.cmbDomain->mapToGlobal(ui.cmbDomain->rect().topRight());
			point.setY(point.y() + ui.cmbDomain->height() / 2);
#endif
		}
		else
		{
			point = ui.lnePassword->mapToGlobal(ui.lnePassword->rect().topRight());
			point.setY(point.y() + ui.lnePassword->height() / 2);
		}
		if (isActiveWindow() || (parentWidget() && parentWidget()->isActiveWindow()))
			BalloonTip::showBalloon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_OPTIONS_ERROR_ALERT), FConnectionErrorWidget, point, 0, true, BalloonTip::ArrowLeft, parentWidget() ? parentWidget() : this);
	}
	else if (FActiveErrorType == ActiveConnectionError)
	{
		QPoint point = ui.pbtConnect->mapToGlobal(ui.pbtConnect->rect().topLeft());
		point.setY(point.y() + ui.pbtConnect->height() / 2);
		if (isActiveWindow() || (parentWidget() && parentWidget()->isActiveWindow()))
			BalloonTip::showBalloon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_OPTIONS_ERROR_ALERT), FConnectionErrorWidget, point, 0, true, BalloonTip::ArrowRight, parentWidget() ? parentWidget() : this);
	}
}

void LoginDialog::hideErrorBallon()
{
	FActiveErrorType = NoActiveError;
	BalloonTip::hideBalloon();
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
						LogDetail(QString("[LoginDialog] Trying IExplorer connection proxy"));
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
						LogDetail(QString("[LoginDialog] Trying FireFox connection proxy"));
						IConnectionProxy proxy = FConnectionManager->proxyById(FIREFOX_PROXY_REF_UUID);
						defConnection->setProxy(proxy.proxy);
						return true;
					}
					return tryNextConnectionSettings();
				}
				else
				{
					LogDetail(QString("[LoginDialog] Reset connection proxy to default"));
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
		hideErrorBallon();
		QTimer::singleShot(3000,this,SLOT(onShowCancelButton()));
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

	ui.pbtConnect->setEnabled(AEnabled);
	ui.pbtConnect->setProperty("connecting", !AEnabled);
	ui.pbtConnect->setText(AEnabled ? tr("Enter") : tr("Connecting..."));
	StyleStorage::updateStyle(this);
}

void LoginDialog::stopReconnection()
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
	if (FStatusChanger && account && account->isActive())
		FStatusChanger->setStreamStatus(account->xmppStream()->streamJid(),STATUS_OFFLINE);

	FReconnectTimer.stop();
	ui.lblReconnect->setText(QString::null);
}

void LoginDialog::showConnectionSettings()
{
	hideConnectionError();
	hideXmppStreamError();
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
	hideErrorBallon();
	stopReconnection();
	ui.lblConnectError->setVisible(false);
	ui.lblReconnect->setVisible(false);
	ui.lblConnectSettings->setVisible(false);
}

void LoginDialog::showConnectionError(const QString &ACaption, const QString &AError)
{
	LogError(QString("[LoginDialog] Connection error '%1': %2").arg(ACaption,AError));
	ReportError("CONNECT-ERROR",QString("[LoginDialog] Connection error '%1': %2").arg(ACaption,AError));

	hideXmppStreamError();

	QString message = ACaption;
	message += message.isEmpty() || AError.isEmpty() ? AError : "<br>" + AError;
	ui.lblConnectError->setText(message);

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

	FActiveErrorType = ActiveConnectionError;
	showErrorBalloon();
}

void LoginDialog::hideXmppStreamError()
{
	hideErrorBallon();
	stopReconnection();
	ui.lneNode->setProperty("error", false);
	ui.lnePassword->setProperty("error", false);
	ui.cmbDomain->setProperty("error", false);
	ui.tlbDomain->setProperty("error", false);
	StyleStorage::updateStyle(this);
	ui.lblXmppError->setVisible(false);
}

void LoginDialog::showXmppStreamError(const QString &ACaption, const QString &AError, const QString &AHint, bool showPasswordEnabled)
{
	LogError(QString("[LoginDialog] Stream error '%1': %2").arg(ACaption, AHint));
	hideConnectionError();

	QString message = ACaption;
	message += message.isEmpty() || AError.isEmpty() ? AError : "<br>" + AError;
	message += message.isEmpty() || AHint.isEmpty() ? AHint : "<br>" + AHint;
	ui.lblXmppError->setText(message);

	if (FNewProfile)
	{
		ui.lneNode->setProperty("error", true);
		ui.cmbDomain->setProperty("error", true);
		ui.tlbDomain->setProperty("error", true);
	}
	ui.lnePassword->setProperty("error", true);
	StyleStorage::updateStyle(this);
	ui.lblXmppError->setVisible(true);
	ui.chbShowPassword->setVisible(showPasswordEnabled);

	FActiveErrorType = ActiveXmppError;
	showErrorBalloon();
}

void LoginDialog::saveCurrentProfileSettings()
{
	QString profile = Jid::encode(currentStreamJid().pBare());
	QMap<QString,QVariant> data = FOptionsManager->profileData(profile);
	if (ui.chbSavePassword->isChecked())
		data.insert("password",QString::fromLatin1(Options::encrypt(ui.lnePassword->text(),FOptionsManager->profileKey(profile,QString::null))));
	else
		data.remove("password");
	data.insert("auto-run",ui.chbAutoRun->isChecked());
	FOptionsManager->setProfileData(profile,data);
	Options::node(OPV_MISC_AUTOSTART).setValue(ui.chbAutoRun->isChecked());
}

void LoginDialog::loadCurrentProfileSettings()
{
	QString profile = Jid::encode(currentStreamJid().pBare());
	QMap<QString,QVariant> data = FOptionsManager->profileData(profile);

	if (data.contains("password"))
	{
		FSavedPasswordCleared = false;
		ui.chbSavePassword->setChecked(true);
		ui.chbShowPassword->setChecked(false);
		ui.lnePassword->setEchoMode(QLineEdit::Password);
		ui.lnePassword->setText(Options::decrypt(data.value("password").toString().toLatin1(),FOptionsManager->profileKey(profile,QString::null)).toString());
	}
	else
	{
		FSavedPasswordCleared = true;
		ui.chbSavePassword->setChecked(false);
		ui.lnePassword->setText(QString::null);
	}

	ui.chbAutoRun->setChecked(data.value("auto-run").toBool());
}

bool LoginDialog::readyToConnect() const
{
	return ui.chbSavePassword->isChecked() && !ui.lnePassword->text().isEmpty();
}

void LoginDialog::onConnectClicked()
{
	if (!ui.pbtConnect->property("connecting").toBool())
	{
		hideConnectionError();
		hideXmppStreamError();
		ui.lneNode->completer()->popup()->hide();
		bool connecting = false;
		setConnectEnabled(false);
		QApplication::processEvents();

		LogDetail(QString("[LoginDialog] Starting login"));

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
							if (!FNewProfile)
								saveCurrentProfileSettings();

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
	else
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
		if (account && account->isActive())
		{
			LogDetail(QString("[LoginDialog] Terminating login"));
			FAbortTimer.start(ABORT_TIMEOUT);
			account->xmppStream()->close();
		}
	}
}

void LoginDialog::onAbortTimerTimeout()
{
	if (ui.pbtConnect->property("connecting").toBool())
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
		if (account && account->isActive())
		{
			account->xmppStream()->abort(tr("Connection terminated by user"));
			showConnectionError(tr("Unable to connect to server"),account->xmppStream()->errorString());
			stopReconnection();
			setConnectEnabled(true);
		}
	}
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
	else if (account && !account->xmppStream()->errorString().isEmpty())
	{
		showXmppStreamError(tr("The password is not suited to login"),QString::null,tr("Check keyboard layout"));
	}
	else
	{
		ui.lblConnectSettings->setVisible(true);
	}

	FAbortTimer.stop();
	FFirstConnect = false;
	setConnectEnabled(true);

	WidgetManager::alertWidget(parentWidget() ? parentWidget() : this);
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
	int domainIndex = ui.cmbDomain->findData(streamJid.pDomain());
	if (!streamJid.pDomain().isEmpty() && domainIndex>=0)
	{
		ui.lneNode->setText(streamJid.node());
		ui.cmbDomain->setCurrentIndex(domainIndex);
		ui.tlbDomain->setText("@"+streamJid.pDomain());
		ui.tlbDomain->setProperty("domain", streamJid.pDomain());
	}
}

void LoginDialog::onCompleterActivated(const QString &AText)
{
	onCompleterHighLighted(AText);
	loadCurrentProfileSettings();
}

void LoginDialog::onDomainCurrentIntexChanged(int AIndex)
{
	hideXmppStreamError();
	hideConnectionError();
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
	{
		FDomainPrevIndex = AIndex;
	}
}

void LoginDialog::onDomainActionTriggered()
{
	hideXmppStreamError();
	hideConnectionError();
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
	ui.cmbDomain->setCurrentIndex(FDomainPrevIndex);
}

void LoginDialog::onLabelLinkActivated(const QString &ALink)
{
	if (ALink == "ramblercontacts.connection.settings")
		showConnectionSettings();
	else
		QDesktopServices::openUrl(ALink);
}

void LoginDialog::onLoginOrPasswordTextChanged()
{
	hideConnectionError();
	hideXmppStreamError();
	ui.pbtConnect->setEnabled(!ui.lneNode->text().isEmpty() && !ui.lnePassword->text().isEmpty());
}

void LoginDialog::onShowCancelButton()
{
	if (ui.pbtConnect->property("connecting").toBool())
	{
		ui.pbtConnect->setEnabled(true);
		ui.pbtConnect->setText(tr("Cancel"));
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.pbtConnect,MNI_OPTIONS_LOGIN_ANIMATION);
	}
}

void LoginDialog::onAdjustDialogSize()
{
	if (parentWidget())
		parentWidget()->adjustSize();
	else
		adjustSize();
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
