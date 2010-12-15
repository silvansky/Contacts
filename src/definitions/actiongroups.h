#ifndef DEF_ACTIONGROUPS_H
#define DEF_ACTIONGROUPS_H

//MainWindow - MainMenu
#define AG_MMENU_RCHAGER_ADD_GROUP                              300
#define AG_MMENU_RCHAGER_ADD_CONTACT                            300
#define AG_MMENU_RCHAGER_ADD_ACCOUNT                            300
#define AG_MMENU_ROSTERSVIEW_SHOWOFFLINE                        500
#define AG_MMENU_ROSTERSVIEW_GROUPCONTACTS                      500
#define AG_MMENU_OPTIONS_SHOWDIALOG                             500
#define AG_MMENU_MASSSEND_SHOWDIALOG                            600
#define AG_MMENU_PLUGINMANAGER_COMMENTS                         700
#define AG_MMENU_PLUGINMANAGER_ABOUT                            700
#define AG_MMENU_CONSOLE_SHOW                                   900
#define AG_MMENU_PLUGINMANAGER_SETUP                            900
#define AG_MMENU_OPTIONS_CHANGEPROFILE                          1000
#define AG_MMENU_MAINWINDOW_QUIT                                1000
//#define AG_MMENU_FILESTREAMSMANAGER                             500
//#define AG_MMENU_ACCOUNTMANAGER                                 500
//#define AG_MMENU_PLUGINMANAGER_SETUP                            800

//RostersView - ContextMenu
#define AG_RVCM_CHATMESSAGEHANDLER                              200
#define AG_RVCM_NORMALMESSAGEHANDLER                            200
#define AG_RVCM_ROSTERCHANGER_GRAND_AUTH                        200
#define AG_RVCM_ROSTERCHANGER_REMOVE_AUTH                       200
#define AG_RVCM_SIPPHONE_CALL                                   200
#define AG_RVCM_ROSTERCHANGER_ADD_CONTACT                       200
#define AG_RVCM_ROSTERSVIEW_GROUP_STATE                         200
#define AG_RVCM_STATUSCHANGER                                   300
#define AG_RVCM_ROSTERCHANGER_GROUP                             300
#define AG_RVCM_GATEWAYS_ADD_LEGACY_USER                        300
#define AG_RVCM_GATEWAYS_LOGIN                                  350
#define AG_RVCM_CLIENTINFO                                      400
#define AG_RVCM_DISCOVERY_FEATURES                              400
#define AG_RVCM_ACCOUNTMANAGER                                  500
#define AG_RVCM_STATUSICONS                                     500
#define AG_RVCM_DISCOVERY                                       500
#define AG_RVCM_MULTIUSERCHAT                                   500
#define AG_RVCM_GATEWAYS_RESOLVE                                500
#define AG_RVCM_PRIVACYLISTS                                    500
#define AG_RVCM_AVATARS                                         500
#define AG_RVCM_ANNOTATIONS                                     500
#define AG_RVCM_ARCHIVER                                        500
#define AG_RVCM_ROSTERSVIEW_GROUPS_STATE                        500
#define AG_RVCM_ROSTERSVIEW_CLIPBOARD                           600
#define AG_RVCM_ROSTERCHANGER_REMOVE_CONTACT                    600
#define AG_RVCM_ROSTERCHANGER_RENAME                            600
#define AG_RVCM_VCARD                                           700

//TrayManager - TrayMenu
#define AG_TMTM_MAINWINDOW                                      300
#define AG_TMTM_NOTIFICATIONS                                   300
#define AG_TMTM_OPTIONS                                         300
#define AG_TMTM_SKINMANAGER                                     500
#define AG_TMTM_DISCOVERY                                       500
#define AG_TMTM_MULTIUSERCHAT                                   500
#define AG_TMTM_BOOKMARKS                                       500
#define AG_TMTM_FILESTREAMSMANAGER                              500
#define AG_TMTM_STATUSCHANGER                                   700
#define AG_TMTM_MESSAGEWIDGETS_LASTTABS                         800
#define AG_TMTM_PLUGINMANAGER                                   900
#define AG_TMTM_TRAYMANAGER                                     1000

//StatusChanger - StatusMenu, StreamMenu
#define AG_SCSM_STATUSCHANGER_ACTIONS                           200
#define AG_SCSM_STATUSCHANGER_STREAMS                           300
#define AG_SCSM_STATUSCHANGER_CUSTOM_STATUS                     400
#define AG_SCSM_STATUSCHANGER_DEFAULT_STATUS                    500
#define AG_SCSM_STATUSCHANGER_CUSTOM_ACTIONS                    700

//MessageWidgets - TabWindowMenu
#define AG_MWTW_MWIDGETS_TAB_ACTIONS                            500
#define AG_MWTW_MWIDGETS_WINDOW_OPTIONS                         800

//MultiuserChat - MultiUserContextMenu
#define AG_MUCM_ROSTERCHANGER                                   500
#define AG_MUCM_CLIENTINFO                                      500
#define AG_MUCM_VCARD                                           500
#define AG_MUCM_STATUSICONS                                     500
#define AG_MUCM_ARCHIVER                                        500
#define AG_MUCM_DISCOVERY                                       500
#define AG_MUCM_DISCOVERY_FEATURES                              600
#define AG_MUCM_MULTIUSERCHAT_UTILS                             800

//MultiUserChat - MultiUserToolsMenu
#define AG_MUTM_MULTIUSERCHAT_COMMON                            300
#define AG_MUTM_MULTIUSERCHAT_TOOLS                             500
#define AG_MUTM_MULTIUSERCHAT_EXIT                              1000

//Bookmarks - BookmarksMenu
#define AG_BMM_BOOKMARKS_STREAMS                                500
#define AG_BMM_BOOKMARKS_ITEMS                                  700
#define AG_BBM_BOOKMARKS_TOOLS                                  900

//ViewWidget - ContextMenu
#define AG_VWCM_MESSAGEWIDGETS_URL                              300
#define AG_VWCM_MESSAGEWIDGETS_COPY                             500
#define AG_VWCM_MESSAGEWIDGETS_QUOTE                            500
#define AG_VWCM_MESSAGEWIDGETS_SEARCH                           700

#endif
