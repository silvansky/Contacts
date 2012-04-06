#!/bin/bash

PROJECT_NAME="virtus.pro"
[ -e $PROJECT_NAME ] || {
	echo "You must execute this script from trunk directory."
	exit 1
}

TMP_DIR="./tmp"
OSX_VER=`sw_vers -productVersion`
VOL_NAME="Рамблер.Контакты"
BG_IMG_NAME="contacts_bg.png"
APP_BUNDLE_NAME="Contacts.app"

[ -d .svn ] && REVISION=".$(sed -n -e '/^dir$/{n;p;q;}' .svn/entries 2>/dev/null)"||REVISION=""
VER_NUMBER="$(grep 'CLIENT_VERSION ' src/definitions/version.h|awk -F'"' '{print $2}')"
VERSION="${VER_NUMBER}${REVISION}"

DMG_NAME_TMP="contacts_${VERSION}_${OSX_VER}_tmp.dmg"
DMG_NAME="contacts_${VERSION}_${OSX_VER}.dmg"

if [[ "$1" != "--nobuild" ]]
then
	echo "*** Calling build script..."
	./src/packages/macosx/make_bundle.sh
fi

echo -n "*** Copying Contacts.app to the temporary dir... "
mkdir $TMP_DIR
cp -R /Applications/Contacts.app $TMP_DIR
#ln -s /Applications $TMP_DIR/Applications
echo "done!"

echo -n "*** Creating temporary dmg disk image..."
rm -f ${DMG_NAME_TMP}
hdiutil create -ov -srcfolder $TMP_DIR -format UDRW -volname ${VOL_NAME} ${DMG_NAME_TMP}

echo -n "*** Mounting temporary image... "
device=$(hdiutil attach -readwrite -noverify -noautoopen ${DMG_NAME_TMP} | egrep '^/dev/' | sed 1q | awk '{print $1}')
echo "done! (${device})"

echo -n "*** Waiting for 5 seconds..."
sleep 5
echo " done!"

echo "*** Setting style for temporary dmg image..."

echo -n "    * Copying background image... "
BG_FOLDER="/Volumes/${VOL_NAME}/.background"
mkdir ${BG_FOLDER}
cp ./resources/${BG_IMG_NAME} ${BG_FOLDER}/
echo "done!"

echo -n "    * Executing applescript for further customization (image will be ejected)... "
echo '
tell application "Finder"
	tell disk "'${VOL_NAME}'"
		open
		set current view of container window to icon view
		set toolbar visible of container window to false
		set statusbar visible of container window to false
		set the bounds of container window to {400, 100, 978, 421}
		set theViewOptions to the icon view options of container window
		set arrangement of theViewOptions to not arranged
		set icon size of theViewOptions to 72
		set background picture of theViewOptions to file ".background:'${BG_IMG_NAME}'"
		make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
		set file_list to every file
		repeat with i in file_list
			if the name of i is "Applications" then
				set the position of i to {425, 145}
			else if the name of i ends with ".app" then
				set the position of i to {150, 150}
			else
				set the position of i to {50, 400}
			end if
			set the label index of i to 7
		end repeat
		close
		open
		set label index of item "Applications" to 7
		set label index of item "'${APP_BUNDLE_NAME}'" to 7
		update without registering applications
		delay 5
		--eject
	end tell
end tell
' | osascript
echo "done!"

echo "*** Converting tempoprary dmg image in compressed readonly final image... "
echo "    * Changing mode and syncing..."
chmod -Rf go-w /Volumes/"${VOL_NAME}"
sync
sync
echo "    * Detaching ${device}..."
hdiutil detach ${device}
rm -f ${DMG_NAME}
echo "    * Converting..."
hdiutil convert "${DMG_NAME_TMP}" -format UDZO -imagekey zlib-level=9 -o "${DMG_NAME}"
echo "done!"

echo -n "*** Removing temporary image... "
rm -f ${DMG_NAME_TMP} 
echo "done!"

echo -n "*** Cleaning up temp folder... "
rm -rf $TMP_DIR
echo "done!"

echo "
*** Everything done. DMG disk image is ready for distribution.
"
