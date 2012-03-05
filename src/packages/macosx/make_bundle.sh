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

SYS_TRANSLATIONS_DIR=`$PATH_TO_QMAKE -query QT_INSTALL_TRANSLATIONS`
SYSTEM_FRAMEWORKS_PATH=/Library/Frameworks

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

function copyframework {
	# copy all
	echo -n "."
	cp -R $SYSTEM_FRAMEWORKS_PATH/$1.framework $APP_FRAMEWORKS_PATH/
	#remove debug libs
	cd $APP_FRAMEWORKS_PATH/$1.framework
	rm -rf $1_debug.dSYM
	rm -f $1_debug
	rm -f $1_debug.prl
	rm -f Headers
	cd Versions/Current/
	rm -f $1_debug
	rm -rf Headers
	# return to apps dir
	cd /Applications
}

echo -n "*** Copying frameworks to bundle..."

copyframework QtCore
copyframework QtGui
copyframework QtNetwork
copyframework QtMultimedia
copyframework QtXml
copyframework QtXmlPatterns
copyframework QtWebKit
copyframework QtDBus
copyframework phonon
copyframework Growl
copyframework Sparkle

echo " done!"

# usage: "patchFile file frameworkName version"
function patchFile {
	# file to patch
	file=$1
	# frmework
	FW=$2
	# version
	VER=$3
	# patching...
	install_name_tool -change $FW.framework/Versions/$VER/$FW @executable_path/../Frameworks/$FW.framework/Versions/$VER/$FW $APPNAME/Contents/$file
}

echo -n "*** Patching frameworks and libraries... "

# Contacts deps - core, network, gui, multimedia, xml, xmlpatterns, webkit, phonon, growl, sparkle

patchFile MacOS/Contacts QtNetwork 4
patchFile MacOS/Contacts QtCore 4
patchFile MacOS/Contacts QtGui 4
patchFile MacOS/Contacts QtMultimedia 4
patchFile MacOS/Contacts QtXml 4
patchFile MacOS/Contacts QtXmlPatterns 4
patchFile MacOS/Contacts QtWebKit 4
patchFile MacOS/Contacts phonon 4
patchFile MacOS/Contacts Growl A
patchFile MacOS/Contacts Sparkle A

# utils deps - core, gui, xml, webkit, network

patchFile Frameworks/libramblercontactsutils.1.dylib QtCore 4
patchFile Frameworks/libramblercontactsutils.1.dylib QtXml 4
patchFile Frameworks/libramblercontactsutils.1.dylib QtGui 4
patchFile Frameworks/libramblercontactsutils.1.dylib QtWebKit 4
patchFile Frameworks/libramblercontactsutils.1.dylib QtNetwork 4

# phonon deps - gui, core, xml, dbus

patchFile Frameworks/phonon.framework/Versions/Current/phonon QtCore 4
patchFile Frameworks/phonon.framework/Versions/Current/phonon QtGui 4
patchFile Frameworks/phonon.framework/Versions/Current/phonon QtXml 4
patchFile Frameworks/phonon.framework/Versions/Current/phonon QtDBus 4

# gui deps - core

patchFile Frameworks/QtGui.framework/Versions/Current/QtGui QtCore 4

# network deps - core

patchFile Frameworks/QtNetwork.framework/Versions/Current/QtNetwork QtCore 4

# mm deps - core, gui

patchFile Frameworks/QtMultimedia.framework/Versions/Current/QtMultimedia QtCore 4
patchFile Frameworks/QtMultimedia.framework/Versions/Current/QtMultimedia QtGui 4

# xml deps - core

patchFile Frameworks/QtXml.framework/Versions/Current/QtXml QtCore 4

# xmlpat deps - core, network

patchFile Frameworks/QtXmlPatterns.framework/Versions/Current/QtXmlPatterns QtCore 4
patchFile Frameworks/QtXmlPatterns.framework/Versions/Current/QtXmlPatterns QtNetwork 4

# dbus deps - xml, core

patchFile Frameworks/QtDBus.framework/Versions/Current/QtDBus QtCore 4
patchFile Frameworks/QtDBus.framework/Versions/Current/QtDBus QtXml 4

# webkit deps - core, gui, xml, dbus, phonon, network

patchFile Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtCore 4
patchFile Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtGui 4
patchFile Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtXml 4
patchFile Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtXmlPatterns 4
patchFile Frameworks/QtWebKit.framework/Versions/Current/QtWebKit phonon 4
patchFile Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtDBus 4
patchFile Frameworks/QtWebKit.framework/Versions/Current/QtWebKit QtNetwork 4

# plugins deps - core, gui, xml, xmlpatterns, network, webkit, phonon, growl, sparkle

for i in $(ls $APPNAME/Contents/PlugIns/); do

	patchFile Plugins/$i QtCore 4
	patchFile Plugins/$i QtGui 4
	patchFile Plugins/$i QtNetwork 4
	patchFile Plugins/$i QtWebKit 4
	patchFile Plugins/$i QtXml 4
	patchFile Plugins/$i QtXmlPatterns 4
	patchFile Plugins/$i phonon 4
	patchFile Plugins/$i Growl A
	patchFile Plugins/$i Sparkle A

done

echo "done!"

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
