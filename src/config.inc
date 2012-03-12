# common definitions
DEFINES                 += LOG_ENABLED
DEFINES                 += WITH_CONSOLE
CONFIG(debug, debug|release) {
  # debug definitions
  DEFINES               += DEBUG_ENABLED
  DEFINES               += WITH_SSEDITOR
} else {
  # release definitions
}

!contains(DEFINES, WITHOUT_PHONON) {
  DEFINES               += USE_PHONON
}

macx: {
  TARGET_LOADER          = Contacts
} else {
  TARGET_LOADER          = ramblercontacts
}

TARGET_UTILS             = ramblercontactsutils
unix:!macx:VERSION_UTILS = 1.2.0

#Configurable values
isEmpty(INSTALL_PREFIX) {
  win32:INSTALL_PREFIX   = "c:"
  unix:!macx: INSTALL_PREFIX   = /usr/local
  macx: INSTALL_PREFIX   = /Applications
}

isEmpty(INSTALL_APP_DIR) {
  win32:INSTALL_APP_DIR  = ramblercontacts
  unix:!macx: INSTALL_APP_DIR  = ramblercontacts
  macx: INSTALL_APP_DIR  = Contacts.app
}

isEmpty(INSTALL_LIB_DIR) {
  win32:INSTALL_LIB_DIR  = .
  unix:!macx: INSTALL_LIB_DIR  = lib
  macx: INSTALL_LIB_DIR  = Frameworks
}

isEmpty(INSTALL_RES_DIR) {
  win32:INSTALL_RES_DIR  = .
  unix:!macx: INSTALL_RES_DIR  = share
  macx: INSTALL_RES_DIR  = Resources
}

#under mac you should install app before debugging it
#(and before running it)
CONFIG(debug, debug|release)!macx {
  DEFINES               += PLUGINS_DIR='\\\"./plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"./resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./src/translations\\\"'
} else:win32 {
  DEFINES               += PLUGINS_DIR='\\\"./$${INSTALL_LIB_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"./$${INSTALL_RES_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./$${INSTALL_RES_DIR}/translations\\\"'
} else:unix:!macx {
  DEFINES               += PLUGINS_DIR='\\\"../$${INSTALL_LIB_DIR}/$${INSTALL_APP_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"../$${INSTALL_RES_DIR}/$${INSTALL_APP_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"../$${INSTALL_RES_DIR}/$${INSTALL_APP_DIR}/translations\\\"'
} else:macx {
  DEFINES               += PLUGINS_DIR='\\\"../PlugIns\\\"'
  DEFINES               += RESOURCES_DIR='\\\"../$${INSTALL_RES_DIR}\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"../$${INSTALL_RES_DIR}/translations\\\"'
}
