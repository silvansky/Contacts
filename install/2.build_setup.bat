@echo off

If "%1%"=="" goto error_specify_ver

SET VER=%1%

del ".\output\%VER%\ContactsSetup.exe"
makensis.exe /DFILE_NAME="../output/%VER%/contactssetup.%VER%.exe" /DPRODUCT_GUID="{9732304B-B640-4C54-B2CD-3C2297D649A1}" /DPRODUCT_EXE="$LOCALAPPDATA\Rambler\Contacts\ramblercontacts.exe" "nsis/setup.nsi" > output/%VER%/log_nsis.txt

exit 0

:error_specify_ver
echo Не указана версия продукта
echo Запуск compile_msi версия 
echo например compile_setup 0.1.0.868
exit 1
