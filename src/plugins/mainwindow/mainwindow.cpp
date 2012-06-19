#include "mainwindow.h"

#include <QKeyEvent>

MainWindow::MainWindow(QWidget *AParent, Qt::WindowFlags AFlags) : QMainWindow(AParent,AFlags)
{
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_MAINWINDOW_WINDOW);

	CustomBorderContainer *border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_ROSTER);
	if (border)
	{
		border->setAttribute(Qt::WA_DeleteOnClose,false);
		border->setMaximizeButtonVisible(false);
		border->setMinimizeButtonVisible(false);
		border->setDockingEnabled(true);
		border->installEventFilter(this);
	}
	else
	{
		setAttribute(Qt::WA_DeleteOnClose,false);
	}
	
	QIcon icon;
	IconStorage *iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO16), QSize(16,16));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO24), QSize(24,24));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO32), QSize(32,32));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO48), QSize(48,48));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO64), QSize(64,64));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO96), QSize(96,96));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO128), QSize(128,128));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO256), QSize(256,256));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO512), QSize(512,512));
	setWindowIcon(icon);

	setIconSize(QSize(16,16));
	createLayouts();
	createToolBars();
	createMenus();
}

MainWindow::~MainWindow()
{

}

bool MainWindow::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

Menu *MainWindow::mainMenu() const
{
	return FMainMenu;
}

QVBoxLayout *MainWindow::mainLayout() const
{
	return FMainLayout;
}

QStackedWidget *MainWindow::upperWidget() const
{
	return FUpperWidget;
}

QStackedWidget *MainWindow::rostersWidget() const
{
	return FRostersWidget;
}

QStackedWidget *MainWindow::bottomWidget() const
{
	return FBottomWidget;
}

IInternalNoticeWidget *MainWindow::noticeWidget() const
{
	return FNoticeWidget;
}

ToolBarChanger *MainWindow::topToolBarChanger() const
{
	return FTopToolBarChanger;
}

ToolBarChanger *MainWindow::leftToolBarChanger() const
{
	return FLeftToolBarChanger;
}

ToolBarChanger *MainWindow::statusToolBarChanger() const
{
	return FStatusToolBarChanger;
}

QMenu *MainWindow::createPopupMenu()
{
	return NULL;
}

void MainWindow::createLayouts()
{
	FUpperWidget = new QStackedWidget(this);
	FUpperWidget->setObjectName("upperWidget");
	FUpperWidget->setVisible(false);
	FUpperWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	connect(FUpperWidget,SIGNAL(currentChanged(int)),SLOT(onStackedWidgetChanged(int)));

	FRostersWidget = new QStackedWidget(this);
	FRostersWidget->setObjectName("rostersWidget");
	FRostersWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	FBottomWidget = new QStackedWidget(this);
	FBottomWidget->setObjectName("bottomWidget");
	FBottomWidget->setVisible(false);
	FBottomWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	connect(FBottomWidget,SIGNAL(currentChanged(int)),SLOT(onStackedWidgetChanged(int)));

	FNoticeWidget = new InternalNoticeWidget(this);
	FNoticeWidget->setObjectName("noticeWidget");
	FNoticeWidget->setVisible(false);

	FMainLayout = new QVBoxLayout;
	FMainLayout->setMargin(0);
	FMainLayout->setSpacing(0);
	FMainLayout->addWidget(FUpperWidget);
	FMainLayout->addWidget(FRostersWidget);
	FMainLayout->addWidget(FBottomWidget);
	FMainLayout->addWidget(FNoticeWidget);
	FMainLayout->setStretch(1, 10);

	QWidget *centralWidget = new QWidget(this);
	centralWidget->setLayout(FMainLayout);
	setCentralWidget(centralWidget);
}

void MainWindow::createToolBars()
{
	QToolBar *toolbar =  new QToolBar(tr("Status toolbar"), this);
	toolbar->setFloatable(false);
	toolbar->setMovable(false);
	toolbar->setObjectName("statusToolBar");
	addToolBar(Qt::TopToolBarArea, toolbar);
#ifdef Q_WS_MAC
	toolbar->layout()->setContentsMargins(0, 0, 0, 0);
#endif
	FStatusToolBarChanger = new ToolBarChanger(toolbar);
	FStatusToolBarChanger->setSeparatorsVisible(false);

	toolbar = new QToolBar(tr("Top toolbar"), this);
	toolbar->setFloatable(false);
	toolbar->setMovable(false);
	addToolBar(Qt::TopToolBarArea,toolbar);
	FTopToolBarChanger = new ToolBarChanger(toolbar);
	FTopToolBarChanger->setSeparatorsVisible(false);
	insertToolBarBreak(toolbar);

	toolbar = new QToolBar(tr("Left toolbar"), this);
	toolbar->setFloatable(false);
	toolbar->setMovable(false);
	addToolBar(Qt::LeftToolBarArea,toolbar);
	FLeftToolBarChanger = new ToolBarChanger(toolbar);
	FLeftToolBarChanger->setSeparatorsVisible(false);
}

void MainWindow::createMenus()
{
	FMainMenu = new Menu(this);
	FMainMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_MENU);
	connect(FMainMenu, SIGNAL(aboutToShow()), SLOT(onMainMenuAboutToShow()));
	connect(FMainMenu, SIGNAL(aboutToHide()), SLOT(onMainMenuAboutToHide()));

#if (!defined(Q_WS_MAC)) || defined(DEBUG_ENABLED)
	QToolButton *button = FTopToolBarChanger->insertAction(FMainMenu->menuAction(), TBG_MWTTB_MAINWINDOW_MAINMENU);
	button->setObjectName("mainMenuButton");
	button->setPopupMode(QToolButton::InstantPopup);
#endif
}

void MainWindow::closeEvent(QCloseEvent *AEvent)
{
	QMainWindow::closeEvent(AEvent);
	emit closed();
}

void MainWindow::onStackedWidgetChanged(int AIndex)
{
	QStackedWidget *widget = qobject_cast<QStackedWidget *>(sender());
	if (AIndex >= 0)
	{
		widget->setMaximumHeight(widget->currentWidget()->sizeHint().height());
		widget->setVisible(true);
	}
	else
	{
		widget->setVisible(false);
	}
}

void MainWindow::onMainMenuAboutToShow()
{
	FMainMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_MENU, 1);
}

void MainWindow::onMainMenuAboutToHide()
{
	FMainMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_MENU);
}
