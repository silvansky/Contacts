@echo off

If "%1%"=="" goto error_specify_ver
If "%2%"=="" goto error_specify_source

set VER=%1%
set SOURCE=%2%

mkdir "output\%VER%"
del ".\output\%VER%\contacts.msi"

echo ^<Include^>                              	 	 > wxs/Product.wxs
echo   ^<?define virtus_version="%VER%" ?^>    		>> wxs/Product.wxs
echo   ^<?define VirtusProjectRoot="%SOURCE%" ?^> 	>> wxs/Product.wxs
type wxs\Product.wxs.tmpl                        	>> wxs/Product.wxs

touch %VER%/*

candle.exe -dVersion=%VER% -out output/%VER%/obj/license.wixobj wxs/License.wxs
candle.exe -dVersion=%VER% -out output/%VER%/obj/Qt.wixobj wxs/Qt.wxs 
candle.exe -dVersion=%VER% -out output/%VER%/obj/OpenSSL.wixobj wxs/OpenSSL.wxs
candle.exe -dVersion=%VER% -out output/%VER%/obj/virtus_imageformats.wixobj wxs/virtus_imageformats.wxs
candle.exe -dVersion=%VER% -out output/%VER%/obj/virtus_plugins6.wixobj wxs/virtus_plugins6.wxs
candle.exe -dVersion=%VER% -out output/%VER%/obj/virtus_resources7.wixobj wxs/virtus_resources7.wxs
candle.exe -dVersion=%VER% -out output/%VER%/obj/virtus_translations6.wixobj wxs/virtus_translations6.wxs
candle.exe -dVersion=%VER% -out output/%VER%/obj/VirtusBase.wixobj wxs/VirtusBase.wxs
candle.exe -dVersion=%VER% -out output/%VER%/obj/Virtus.wixobj wxs/Virtus.wxs
candle.exe -dVersion=%VER% -out output/%VER%/obj/ms.wixobj wxs/ms.wxs
candle.exe -dVersion=%VER% -out output/%VER%/obj/voip.wixobj wxs/voip.wxs

light -out output/%VER%/obj/contacts.wixpdb output/%VER%/obj/OpenSSL.wixobj output/%VER%/obj/license.wixobj output/%VER%/obj/Qt.wixobj output/%VER%/obj/virtus_imageformats.wixobj output/%VER%/obj/virtus_plugins6.wixobj output/%VER%/obj/virtus_resources7.wixobj output/%VER%/obj/virtus_translations6.wixobj output/%VER%/obj/VirtusBase.wixobj output/%VER%/obj/Virtus.wixobj output/%VER%/obj/ms.wixobj output/%VER%/obj/voip.wixobj -o output/%VER%/contacts.%VER%.msi > output/%VER%/log_msi.txt
exit 0

:error_specify_ver
echo Не указана версия продукта
echo Запуск compile_msi версия путь-к-файлам
echo например compile_msi 0.1.0.868 c:/rambler/virtus/trunk/install_dir
exit 1

:error_specify_source
echo не указаны исходные файлы
echo Запуск compile_msi версия путь-к-файлам
echo например compile_msi 0.1.0.868 c:/rambler/virtus/trunk/install_dir
exit 2