#ifndef DEF_OPTIONVALUES_H
#define DEF_OPTIONVALUES_H

// AccountManager
#define OPV_ACCOUNT_ROOT                                "accounts"
#define OPV_ACCOUNT_ITEM                                "accounts.account"
#define OPV_ACCOUNT_NAME                                "accounts.account.name"
#define OPV_ACCOUNT_ACTIVE                              "accounts.account.active"
#define OPV_ACCOUNT_STREAMJID                           "accounts.account.streamJid"
#define OPV_ACCOUNT_PASSWORD                            "accounts.account.password"
// ConnectionManager
#define OPV_ACCOUNT_CONNECTION_ITEM                     "accounts.account.connection"
#define OPV_ACCOUNT_CONNECTION_TYPE                     "accounts.account.connection-type"
// DefaultConnection
#define OPV_ACCOUNT_CONNECTION_HOST                     "accounts.account.connection.host"
#define OPV_ACCOUNT_CONNECTION_PORT                     "accounts.account.connection.port"
#define OPV_ACCOUNT_CONNECTION_PROXY                    "accounts.account.connection.proxy"
#define OPV_ACCOUNT_CONNECTION_USESSL                   "accounts.account.connection.use-ssl"
#define OPV_ACCOUNT_CONNECTION_IGNORESSLERRORS          "accounts.account.connection.ignore-ssl-errors"
// StatusChanger
#define OPV_ACCOUNT_AUTOCONNECT                         "accounts.account.auto-connect"
#define OPV_ACCOUNT_AUTORECONNECT                       "accounts.account.auto-reconnect"
#define OPV_ACCOUNT_STATUS_ISMAIN                       "accounts.account.status.is-main"
#define OPV_ACCOUNT_STATUS_LASTONLINE                   "accounts.account.status.last-online"

//BirthdayReminder
#define OPV_BIRTHDAY_NOTICE_SHOWCOUNT                   "birthday.notice.show-count"
#define OPV_BIRTHDAY_NOTICE_SHOWLAST                    "birthday.notice.show-last"

// Console
#define OPV_CONSOLE_ROOT                                "console"
#define OPV_CONSOLE_CONTEXT_ITEM                        "console.context"
#define OPV_CONSOLE_CONTEXT_NAME                        "console.context.name"
#define OPV_CONSOLE_CONTEXT_STREAMJID                   "console.context.streamjid"
#define OPV_CONSOLE_CONTEXT_CONDITIONS                  "console.context.conditions"
#define OPV_CONSOLE_CONTEXT_HIGHLIGHTXML                "console.context.highlight-xml"
#define OPV_CONSOLE_CONTEXT_WORDWRAP                    "console.context.word-wrap"

//Gateways
#define OPV_GATEWAYS_NOTICE_SHOWCOUNT                   "gateways.notice.show-count"
#define OPV_GATEWAYS_NOTICE_SHOWLAST                    "gateways.notice.show-last"
#define OPV_GATEWAYS_NOTICE_REMOVECOUNT                 "gateways.notice.remove-count"

// MainWindow
#define OPV_MAINWINDOW_SHOW                             "mainwindow.show"
#define OPV_MAINWINDOW_SIZE                             "mainwindow.size"
#define OPV_MAINWINDOW_POSITION                         "mainwindow.position"
#define OPV_MAINWINDOW_STAYONTOP                        "mainwindow.stay-on-top"
#define OPV_MAINWINDOW_MINIMIZETOTRAY_W7                "mainwindow.minimize-to-tray-w7"

// MessageWidgets
#define OPV_MESSAGES_ROOT                               "messages"
#define OPV_MESSAGES_SHOWSTATUS                         "messages.show-status-changes"
#define OPV_MESSAGES_SHOWINFOWIDGET                     "messages.show-info-widget"
#define OPV_MESSAGES_LASTTABPAGESCOUNT                  "messages.last-tab-pages-count"
#define OPV_MESSAGES_EDITORAUTORESIZE                   "messages.editor-auto-resize"
#define OPV_MESSAGES_EDITORMINIMUMLINES                 "messages.editor-minimum-lines"
#define OPV_MESSAGES_EDITORMAXIMUMLINES                 "messages.editor-maximum-lines"
#define OPV_MESSAGES_EDITORSENDKEY                      "messages.editor-send-key"
#define OPV_MESSAGES_CLEANCHATTIMEOUT                   "messages.clean-chat-timeout"
#define OPV_MESSAGES_TABWINDOWS_ROOT                    "messages.tab-windows"
#define OPV_MESSAGES_TABWINDOWS_ENABLE                  "messages.tab-windows.enable"
#define OPV_MESSAGES_TABWINDOWS_DEFAULT                 "messages.tab-windows.default"
#define OPV_MESSAGES_TABWINDOW_ITEM                     "messages.tab-windows.window"
#define OPV_MESSAGES_TABWINDOW_NAME                     "messages.tab-windows.window.name"
// Emoticons
#define OPV_MESSAGES_EMOTICONS                          "messages.emoticons"
#define OPV_MESSAGES_EMOTICONS_ENABLED                  "messages.emoticons.enabled"
// ChatStates
#define OPV_MESSAGES_CHATSTATESENABLED                  "messages.chatstates-enabled"
//MessageStyles
#define OPV_MESSAGES_SHOWDATESEPARATORS                 "messages.show-date-separators"

// MessageStyles
#define OPV_MESSAGESTYLE_ROOT                           "message-styles"
#define OPV_MESSAGESTYLE_MTYPE_ITEM                     "message-styles.message-type"
#define OPV_MESSAGESTYLE_CONTEXT_ITEM                   "message-styles.message-type.context"
#define OPV_MESSAGESTYLE_STYLE_TYPE                     "message-styles.message-type.context.style-type"
#define OPV_MESSAGESTYLE_STYLE_ITEM                     "message-styles.message-type.context.style"
// AdiumMessageStyle
#define OPV_MESSAGESTYLE_STYLE_ID                       "message-styles.message-type.context.style.style-id"
#define OPV_MESSAGESTYLE_STYLE_VARIANT                  "message-styles.message-type.context.style.variant"
#define OPV_MESSAGESTYLE_STYLE_FONTFAMILY               "message-styles.message-type.context.style.font-family"
#define OPV_MESSAGESTYLE_STYLE_FONTSIZE                 "message-styles.message-type.context.style.font-size"
#define OPV_MESSAGESTYLE_STYLE_BGCOLOR                  "message-styles.message-type.context.style.bg-color"
#define OPV_MESSAGESTYLE_STYLE_BGIMAGEFILE              "message-styles.message-type.context.style.bg-image-file"
#define OPV_MESSAGESTYLE_STYLE_BGIMAGELAYOUT            "message-styles.message-type.context.style.bg-image-layout"

// MultiUserChat
#define OPV_MUC_GROUPCHAT_SHOWENTERS                    "muc.groupchat.show-enters"
#define OPV_MUC_GROUPCHAT_SHOWSTATUS                    "muc.groupchat.show-status"
#define OPV_MUC_GROUPCHAT_ARCHIVESTATUS                 "muc.groupchat.archive-status"

// OptionsManager
#define OPV_MISC_ROOT                                   "misc"
#define OPV_MISC_AUTOSTART                              "misc.autostart"
#define OPV_MISC_SHAREOSVERSION                         "misc.share-os-version"
#define OPV_MISC_OPTIONS_SAVE_ON_SERVER                 "misc.options.save-on-server"
#define OPV_MISC_OPTIONS_DIALOG_LASTNODE                "misc.options.dialog.last-node"
// RamblerHistory
#define OPV_MISC_HISTORY_SAVE_ON_SERVER                 "misc.history.save-on-server"

// Notifications
#define OPV_NOTIFICATIONS_ROOT                          "notifications"
#define OPV_NOTIFICATIONS_NONOTIFYIFDND                 "notifications.no-notify-if-dnd"
#define OPV_NOTIFICATIONS_NONOTIFYIFAWAY                "notifications.no-notify-if-away"
#define OPV_NOTIFICATIONS_NONOTIFYIFFULLSCREEN          "notifications.no-notify-if-fullscreen"
#define OPV_NOTIFICATIONS_SOUND_COMMAND                 "notifications.sound-command"
#define OPV_NOTIFICATIONS_TYPEKINDS_ROOT                "notifications.type-kinds"
#define OPV_NOTIFICATIONS_TYPEKINDS_ITEM                "notifications.type-kinds.type"
#define OPV_NOTIFICATIONS_KINDENABLED_ROOT              "notifications.kind-enabled"
#define OPV_NOTIFICATIONS_KINDENABLED_ITEM              "notifications.kind-enabled.kind"

// ConnectionManager
#define OPV_PROXY_ROOT                                  "proxy"
#define OPV_PROXY_DEFAULT                               "proxy.default"
#define OPV_PROXY_ITEM                                  "proxy.proxy"
#define OPV_PROXY_NAME                                  "proxy.proxy.name"
#define OPV_PROXY_TYPE                                  "proxy.proxy.type"
#define OPV_PROXY_HOST                                  "proxy.proxy.host"
#define OPV_PROXY_PORT                                  "proxy.proxy.port"
#define OPV_PROXY_USER                                  "proxy.proxy.user"
#define OPV_PROXY_PASS                                  "proxy.proxy.pass"

// RostersView
#define OPV_ROSTER_ROOT                                 "roster"
#define OPV_ROSTER_SHOWOFFLINE                          "roster.show-offline"
#define OPV_ROSTER_SHOWRESOURCE                         "roster.show-resource"
#define OPV_ROSTER_SHOWSTATUSTEXT                       "roster.show-status-text"
#define OPV_ROSTER_SORTBYNAME                           "roster.sort-by-name"
#define OPV_ROSTER_SORTBYSTATUS                         "roster.sort-by-status"
#define OPV_ROSTER_SORTBYHAND                           "roster.sort-by-hand"
#define OPV_ROSTER_GROUPCONTACTS                        "roster.group-contacts"
// RosterChanger
#define OPV_ROSTER_AUTOSUBSCRIBE                        "roster.auto-subscribe"
#define OPV_ROSTER_AUTOUNSUBSCRIBE                      "roster.auto-unsubscribe"
#define OPV_ROSTER_ADDCONTACTDIALOG_LASTGROUP           "roster.add-contact-dialog.last-group"
// Avatars
#define OPV_AVATARS_SHOW                                "roster.avatars.show"
#define OPV_AVATARS_SHOWEMPTY                           "roster.avatars.show-empty"

// StatusChanger
#define OPV_STATUSES_ROOT                               "statuses"
#define OPV_STATUSES_MAINSTATUS                         "statuses.main-status"
#define OPV_STATUS_ITEM                                 "statuses.status"
#define OPV_STATUS_NAME                                 "statuses.status.name"
#define OPV_STATUS_SHOW                                 "statuses.status.show"
#define OPV_STATUS_TEXT                                 "statuses.status.text"
#define OPV_STATUS_PRIORITY                             "statuses.status.priority"
// AutoStatus
#define OPV_AUTOSTARTUS_ROOT                            "statuses.autostatus"
#define OPV_AUTOSTARTUS_AWAYONLOCK                      "statuses.autostatus.away-on-lock"
#define OPV_AUTOSTARTUS_RULE_ITEM                       "statuses.autostatus.rule"
#define OPV_AUTOSTARTUS_RULE_ENABLED                    "statuses.autostatus.rule.enabled"
#define OPV_AUTOSTARTUS_RULE_TIME                       "statuses.autostatus.rule.time"
#define OPV_AUTOSTARTUS_RULE_SHOW                       "statuses.autostatus.rule.show"
#define OPV_AUTOSTARTUS_RULE_TEXT                       "statuses.autostatus.rule.text"

// StatusIcons
#define OPV_STATUSICONS                                 "statusicons"
#define OPV_STATUSICONS_DEFAULT                         "statusicons.default-iconset"
#define OPV_STATUSICONS_RULES_ROOT                      "statusicons.rules"
#define OPV_STATUSICONS_RULE_ITEM                       "statusicons.rules.rule"
#define OPV_STATUSICONS_RULE_PATTERN                    "statusicons.rules.rule.pattern"
#define OPV_STATUSICONS_RULE_ICONSET                    "statusicons.rules.rule.iconset"

#endif // DEF_OPTIONVALUES_H
