@echo off

If "%1%"=="" goto error_specify_source

set SOURCE=%1%

heat dir %SOURCE%/translations -gg -indent 2 -scom -sreg -dr INSTALLLOCATION directoryid ertyuikuhuihiuhiu -cg Translations -var var.VirtusTranslationsRoot -o virtus_translations6.wxs
heat dir %SOURCE%/resources -gg -indent 2 -scom -sreg -dr INSTALLLOCATION -cg Resources5 -var var.VirtusResourcesRoot -o virtus_resources7.wxs
heat dir %SOURCE%/plugins -gg -indent 2 -scom -sreg -dr INSTALLLOCATION -cg Plugins -var var.VirtusPluginsRoot -template fragment -o virtus_plugins6.wxs
heat dir %SOURCE%/imageformats -gg -indent 2 -scom -sreg -dr INSTALLLOCATION -cg Imageformats -var var.VirtusImageformatsRoot -o virtus_imageformats.wxs


exit 0

:error_specify_ver
echo Не указаны исходные файлы
exit 1