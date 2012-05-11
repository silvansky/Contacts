#!/bin/bash

# IMPORTANT! Path to qmake of Qt installation, against which project was built.
PATH_TO_QMAKE="qmake"
QMAKE_FLAGS="-r -spec macx-g++ CONFIG+=release CONFIG-=debug CONFIG-=debug_and_release DEFINES+=USE_PHONON $@"
PROJECT_NAME="virtus.pro"
PATH_TO_MAKE="make"
MAKE_FLAGS="-j4 -s -w"
MAKE_INSTALL_FLAGS="-s install"
MAKE_CLEAN_FLAGS="-s clean"

[ -e $PROJECT_NAME ]|| {
	echo "You must execute this script from trunk directory."
	exit 1
}

APPNAME=Contacts.app
APP_FRAMEWORKS_PATH=$APPNAME/Contents/Frameworks
APP_PLUGINS_PATH=$APPNAME/Contents/PlugIns

SYS_TRANSLATIONS_DIR=`$PATH_TO_QMAKE -query QT_INSTALL_TRANSLATIONS`
SYS_PLUGINS_DIR=`$PATH_TO_QMAKE -query QT_INSTALL_PLUGINS`
SYSTEM_FRAMEWORKS_PATH=/Library/Frameworks
SYSTEM_DYLIBS_PATH=/usr/local/lib

if [ -f Makefile ]
then
	read -p "It seems you have already build something here. Do you want to cleanup build (y/n)? " -n 1 -r
	if [[ $REPLY =~ ^[Yy]$ ]]
	then
		echo
		echo -n "*** Cleaning build... "
		$PATH_TO_MAKE $MAKE_CLEAN_FLAGS
		echo "done!"
	fi
fi

echo
echo "*** Building Contacts..."

$PATH_TO_QMAKE $PROJECT_NAME $QMAKE_FLAGS
$PATH_TO_MAKE $MAKE_FLAGS

echo -n "done!
*** Installing Contacts... "

$PATH_TO_MAKE $MAKE_INSTALL_FLAGS
echo "done!"

cd /Applications

# copy qt frameworks

mkdir $APP_FRAMEWORKS_PATH

function copyFramework {
	# copy all
	FW_NAME=$1
	echo -n "."
	cp -R ${SYSTEM_FRAMEWORKS_PATH}/${FW_NAME}.framework ${APP_FRAMEWORKS_PATH}/
	#remove debug libs
	cd ${APP_FRAMEWORKS_PATH}/${FW_NAME}.framework
	echo -n "."
	rm -rf ${FW_NAME}_debug.dSYM
	rm -f ${FW_NAME}_debug
	rm -f ${FW_NAME}_debug.prl
	rm -f Headers
	cd Versions/Current/
	rm -f ${FW_NAME}_debug
	rm -rf Headers
	# return to apps dir
	cd /Applications
	echo -n "."
}

function copyDylib {
	LIB_NAME=lib$1.dylib
	echo -n "."
	cp -R ${SYSTEM_DYLIBS_PATH}/${LIB_NAME} ${APP_FRAMEWORKS_PATH}/
	echo -n "."
	install_name_tool -id ${LIB_NAME} ${APP_FRAMEWORKS_PATH}/${LIB_NAME} 
	echo -n "."
}

echo -n "*** Copying frameworks to bundle..."

copyFramework QtCore
copyFramework QtGui
copyFramework QtNetwork
copyFramework QtMultimedia
copyFramework QtSvg
copyFramework QtXml
copyFramework QtXmlPatterns
copyFramework QtWebKit
copyFramework QtDBus
copyFramework phonon
copyFramework Growl
copyFramework Sparkle

echo " done!"

echo -n "*** Copying dylibs to bundle..."

copyDylib dbus-1.3

echo " done!"

# copy Qt imageformats plugins

function copyImageFormatPlugin {
	echo -n "."
	CODEC=$1
	PLUGIN="${SYS_PLUGINS_DIR}/imageformats/libq${CODEC}.dylib"
	cp ${PLUGIN} $APP_PLUGINS_PATH/imageformats/
	echo -n ".."
}

echo -n "*** Copying imageformats plugins to bundle..."

mkdir $APP_PLUGINS_PATH/imageformats

for codec in "jpeg" "ico" "tga" "tiff" "mng" "svg" "gif"; do
	copyImageFormatPlugin $codec
done

echo " done!"

# usage: "patchFileFW file frameworkName version"
function patchFileFW {
	# file to patch
	file=$1
	# framework
	FW=$2
	# version
	VER=$3
	# patching...
	install_name_tool -change $FW.framework/Versions/$VER/$FW @executable_path/../Frameworks/$FW.framework/Versions/$VER/$FW $APPNAME/Contents/$file
}

# usage: "patchFileDylib file dylibNameWithVersion"
function patchFileDylib {
	# file to patch
	file=$1
	# dylib
	DYLIB=lib$2.dylib
	# patching...
	install_name_tool -change ${SYSTEM_DYLIBS_PATH}/${DYLIB} @executable_path/../Frameworks/${DYLIB} $APPNAME/Contents/$file
}

echo -n "*** Patching frameworks, libraries and plugins..."

# Contacts deps - core, network, gui, multimedia, xml, xmlpatterns, webkit, phonon, growl, sparkle

patchFileFW MacOS/Contacts QtNetwork 4
patchFileFW MacOS/Contacts QtCore 4
patchFileFW MacOS/Contacts QtGui 4
patchFileFW MacOS/Contacts QtMultimedia 4
patchFileFW MacOS/Contacts QtXml 4
patchFileFW MacOS/Contacts QtXmlPatterns 4
patchFileFW MacOS/Contacts QtWebKit 4
patchFileFW MacOS/Contacts phonon 4
patchFileFW MacOS/Contacts Growl A
patchFileFW MacOS/Contacts Sparkle A
echo -n "."

# utils deps - core, gui, xml, webkit, network

patchFileFW Frameworks/libramblercontactsutils.1.dylib QtCore 4
patchFileFW Frameworks/libramblercontactsutils.1.dylib QtXml 4
patchFileFW Frameworks/libramblercontactsutils.1.dylib QtGui 4
patchFileFW Frameworks/libramblercontactsutils.1.dylib QtWebKit 4
patchFileFW Frameworks/libramblercontactsutils.1.dylib QtNetwork 4
echo -n "."

# phonon deps - gui, core, xml, dbus

patchFileFW Frameworks/phonon.framework/Versions/Current/phonon QtCore 4
patchFileFW Frameworks/phonon.framework/Versions/Current/phonon QtGui 4
patchFileFW Frameworks/phonon.framework/Versions/Current/phonon QtXml 4
patchFileFW Frameworks/phonon.framework/Versions/Current/phonon QtDBus 4
echo -n "."

# gui deps - core

patchFileFW Frameworks/QtGui.framework/Versions/Current/QtGui QtCore 4
echo -n "."

# network deps - core

patchFileFW Frameworks/QtNetwork.framework/Versions/Current/QtNetwork QtCore 4
echo -n "."

# mm deps - core, gui

patchFileFW Frameworks/QtMultimedia.framework/Versions/Current/QtMultimedia QtCore 4
patchFileFW Frameworks/QtMultimedia.framework/Versions/Current/QtMultimedia QtGui 4
echo -n "."

# svg deps - core, gui

patchFileFW Frameworks/QtSvg.framework/Versions/Current/QtSvg QtCore 4
patchFileFW Frameworks/QtSvg.framework/Versions/Current/QtSvg QtGui 4
echo -n "."

# xml deps - core

patchFileFW Frameworks/QtXml.framework/Versions/Current/QtXml QtCore 4
echo -n "."

# xmlpat deps - core, network

patchFileFW Frameworks/QtXmlPatterns.framework/Versions/Current/QtXmlPatterns QtCore 4
patchFileFW Frameworks/QtXmlPatterns.framework/Versions/Current/QtXmlPatterns QtNetwork 4
echo -n "."

# dbus deps - xml, core

patchFileFW Frameworks/QtDBus.framework/Versions/Current/QtDBus QtCore 4
patchFileFW Frameworks/QtDBus.framework/Versions/Current/QtDBus QtXml 4
echo -n "."

# webkit deps - core, gui, xml, dbus, phonon, network

patchFileFW Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtCore 4
patchFileFW Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtGui 4
patchFileFW Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtXml 4
patchFileFW Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtXmlPatterns 4
patchFileFW Frameworks/QtWebKit.framework/Versions/Current/QtWebKit phonon 4
patchFileFW Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtDBus 4
patchFileFW Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtNetwork 4
echo -n "."

# plugins deps - core, gui, xml, xmlpatterns, network, webkit, phonon, growl, sparkle

for i in `cd $APPNAME/Contents/PlugIns/ && ls *.dylib`; do
	patchFileFW PlugIns/$i QtCore 4
	patchFileFW PlugIns/$i QtGui 4
	patchFileFW PlugIns/$i QtNetwork 4
	patchFileFW PlugIns/$i QtWebKit 4
	patchFileFW PlugIns/$i QtMultimedia 4
	patchFileFW PlugIns/$i QtXml 4
	patchFileFW PlugIns/$i QtXmlPatterns 4
	patchFileFW PlugIns/$i QtDBus 4
	patchFileFW PlugIns/$i phonon 4
	patchFileFW PlugIns/$i Growl A
	patchFileFW PlugIns/$i Sparkle A
	patchFileDylib PlugIns/$i dbus-1.3
	echo -n "."
done

for i in `cd $APPNAME/Contents/PlugIns/imageformats/ && ls *.dylib`; do
	patchFileFW PlugIns/imageformats/$i QtCore 4
	patchFileFW PlugIns/imageformats/$i QtGui 4
	patchFileFW PlugIns/imageformats/$i QtSvg 4
	echo -n "."
done

echo " done!"

cd $APPNAME

echo -n "*** Copying Qt locales to bundle... "
AVAIBLE_LANGUAGES=`ls Contents/Resources/translations/`
for lang in $AVAIBLE_LANGUAGES ; do
	cp "$SYS_TRANSLATIONS_DIR/qt_$lang.qm" "Contents/Resources/translations/$lang/qt_$lang.qm"
done
echo "done!"

echo -n "*** Removing .svn folders from bundle... "

rm -rf `find . -type d -name .svn`

echo "done!"

echo "Everything done! Now you can create .dmg image with Disk Utility or by calling ./src/packages/macosx/make_dmg.sh --nobuild"
