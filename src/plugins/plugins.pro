TEMPLATE = subdirs

SUBDIRS += optionsmanager
SUBDIRS += xmppstreams
SUBDIRS += iqauth
SUBDIRS += saslauth
SUBDIRS += stanzaprocessor
SUBDIRS += roster
SUBDIRS += presence
SUBDIRS += rostersmodel
SUBDIRS += mainwindow
SUBDIRS += rostersview
SUBDIRS += statuschanger
SUBDIRS += rosterchanger
SUBDIRS += accountmanager
SUBDIRS += traymanager
SUBDIRS += privatestorage
SUBDIRS += messageprocessor
SUBDIRS += messagewidgets
SUBDIRS += messagestyles
SUBDIRS += adiummessagestyle
SUBDIRS += chatmessagehandler
SUBDIRS += compress
SUBDIRS += connectionmanager
SUBDIRS += defaultconnection
SUBDIRS += starttls
SUBDIRS += statusicons
SUBDIRS += emoticons
SUBDIRS += clientinfo
SUBDIRS += vcard
SUBDIRS += dataforms
SUBDIRS += servicediscovery
SUBDIRS += gateways
SUBDIRS += avatars
SUBDIRS += notifications
SUBDIRS += autostatus
SUBDIRS += rostersearch
SUBDIRS += chatstates
SUBDIRS += xmppuriqueries
SUBDIRS += bitsofbinary
SUBDIRS += registration
SUBDIRS += ramblerhistory
SUBDIRS += birthdayreminder
SUBDIRS += metacontacts
SUBDIRS += smsmessagehandler
SUBDIRS += ramblermailnotify
SUBDIRS += messagecarbons
SUBDIRS += console
SUBDIRS += multiuserchat
SUBDIRS += stylesheeteditor

# platform specific plugins

win32-msvc2008: {
  SUBDIRS += sipphone
}

macx: {
  SUBDIRS += macintegration
}
