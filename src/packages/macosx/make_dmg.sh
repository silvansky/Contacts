#!/bin/bash

PROJECT_NAME="virtus.pro"
TMP_DIR="./tmp"

[ -e $PROJECT_NAME ]|| {
	echo "You must execute this script from trunk directory."
	exit 1
}

[ -d .svn ] && REVISION=".$(sed -n -e '/^dir$/{n;p;q;}' .svn/entries 2>/dev/null)"||REVISION=""
VER_NUMBER="$(grep 'CLIENT_VERSION ' src/definitions/version.h|awk -F'"' '{print $2}')"
VERSION="${VER_NUMBER}${REVISION}"

if [[ "$1" != "--nobuild" ]]
then
	echo "*** Calling build script..."
	./src/packages/macosx/make_bundle.sh
fi

echo -n "*** Copying Contacts.app to the temporary dir... "
mkdir $TMP_DIR
cp -R /Applications/Contacts.app $TMP_DIR
ln -s /Applications $TMP_DIR/Applications
echo "done!"

echo -n "*** Creating dmg disk image..."
OSX_VER=`sw_vers -productVersion`
hdiutil create -ov -srcfolder $TMP_DIR -format UDBZ -volname "Рамблер.Контакты" "contacts_${VERSION}_${OSX_VER}.dmg"

echo -n "Cleaning up temp folder... "
rm -rf $TMP_DIR
echo "done!"
