;TargetMinimalOS 5.0
!include LogicLib.nsh
!include UAC.nsh
!include WinVer.nsh
!include nsRIS.nsh
!include nsRUH.nsh

!define PRODUCT_NAME "Рамблер-Контакты"

Name "Рамблер-Контакты"
Caption "Рамблер-Контакты"
BrandingText 'ООО "Рамблер Интернет Холдинг"'

!define MUI_ICON "application.ico"
!define MUI_UNICON "application.ico"
!define MUI_CUSTOMFUNCTION_GUIINIT GuiInit

!define MUI_COMPONENTSPAGE_NODESC
!define MUI_PRODUCT ${PRODUCT_NAME}
!include "MUI2.nsh"
!include "nsDialogs.nsh"

!define MUI_WELCOMEFINISHPAGE_BITMAP "setup.bmp"
!define MUI_ABORTWARNING

!define PBS_MARQUEE 0x08
OutFile ${FILE_NAME}
XPStyle on
Icon "application.ico"
RequestExecutionLevel user

!insertmacro RUH_FUNCTIONS

!ifndef BCM_SETSHIELD
	!define BCM_SETSHIELD 0x0000160C
!endif

page custom welcome_init welcome_exit
!insertmacro MUI_PAGE_LICENSE "license.rtf"
!insertmacro MUI_PAGE_INSTFILES
page custom finish_init finish_exit

!insertmacro MUI_LANGUAGE "Russian" 
!insertmacro MUI_PAGE_FUNCTION_FULLWINDOW

; version information
VIProductVersion $%VER%
VIAddVersionKey /LANG=${LANG_RUSSIAN} "ProductName" ${PRODUCT_NAME}
VIAddVersionKey /LANG=${LANG_RUSSIAN} "CompanyName" 'ООО "Рамблер Интернет Холдинг"'
VIAddVersionKey /LANG=${LANG_RUSSIAN} "FileVersion" "$%VER%	"

Var welcome_dialog
Var welcome_checkbox1
Var welcome_checkbox2
Var finish_checkbox1
Var finish_checkbox2
Var welcome_image
Var finish_image
Var welcome_image_handle
Var finish_image_handle

; utility functions
Function InstallUpdater
  SetOutPath "$TEMP"
  File "..\Holdem\holdem.msi"
  ExecWait '"msiexec" /i "$OUTDIR\holdem.msi" /quiet'
FunctionEnd

Function RelGotoPage
  IntCmp $R9 0 0 Move Move
    StrCmp $R9 "X" 0 Move
      StrCpy $R9 "120"
 
  Move:
    SendMessage $HWNDPARENT "0x408" "$R9" ""
FunctionEnd

Function WriteToFireWall
  nsisFirewallW::AddAuthorizedApplication "${PRODUCT_EXE}" "${PRODUCT_NAME}"
  Pop $0
FunctionEnd

Function welcome_init
	nsDialogs::Create /NOUNLOAD 1044;1044;1018;1044;1018
	Pop $0
     	 
	SetCtlColors welcome_dialog '0x000000' '0xFFFFFF'
	${NSD_CreateBitmap} 0 0 100% 100% ""
	Pop $welcome_image
	${NSD_SetImage} $welcome_image $PLUGINSDIR\image.png $welcome_image_handle

	;${NSD_CreateLabel} 120u 10u 100% 48u "Вас приветствует$\nмастер установки$\n«Рамблер-Контактов».$\n$\n"
	;Pop $0	
	;SetCtlColors $0 '0x000000' transparent

	${NSD_CreateLabel} 120u 10u 60% 50u "Рамблер-Контакты будут установлены на ваш компьютер."
	Pop $0
	
	CreateFont $1 "$(^Font)" "13" "" 
	SendMessage $0 ${WM_SETFONT} $1 0	
	SetCtlColors $0 '0x000000' transparent

	
	; запустить после установки
	${NSD_CreateCheckbox} 120u 60u 100% 20u "Сделать Рамблер домашней страницей"
	Pop $welcome_checkbox1
	SetCtlColors $welcome_checkbox1 '0x000000' transparent

	${NSD_Check} $welcome_checkbox1 
	${NSD_OnClick} $welcome_checkbox1 OnClickCheckAdminRights

	; browser defaults
	${NSD_CreateCheckbox} 120u 80u 100% 20u "Сделать Рамблер поиском по умолчанию"
	Pop $welcome_checkbox2
	SetCtlColors $welcome_checkbox2 '0x000000' transparent

	${NSD_Check} $welcome_checkbox2
	${NSD_OnClick} $welcome_checkbox2 OnClickCheckAdminRights
	
	${RUH_SetShield}
	
	Call muiPageLoadFullWindow
	nsDialogs::Show
	${NSD_FreeImage} $welcome_image_handle
	Call muiPageUnloadFullWindow
	Call ElemBranding
FunctionEnd

Function OnClickCheckAdminRights
	Pop $0 ; don't forget to pop HWND of the stack
	${NSD_GetState} $welcome_checkbox1 $1
	${NSD_GetState} $welcome_checkbox2 $2

	${If} $1 == ${BST_CHECKED}
		${RUH_SetShield}
	${Else}
		${If} $2 == ${BST_CHECKED}
			${RUH_SetShield}
		${Else}
			${RUH_RemShield}
		${EndIf}
	${EndIf}
FunctionEnd

Function welcome_exit
  	Pop $0
	
  	${NSD_GetState} $welcome_checkbox1 $1
  	${If} $1 == ${BST_CHECKED}
    	WriteINIStr $APPDATA\temp.ini "Rambler" "SetHome" 1
    	call RUH_SetAdminIfNeeded 
  	${EndIf}	

	${NSD_GetState} $welcome_checkbox2 $2
  	${If} $2 == ${BST_CHECKED}
    	WriteINIStr $APPDATA\temp.ini "Rambler" "SetSearch" 1
    	call RUH_SetAdminIfNeeded 
  	${EndIf}

FunctionEnd

; custom page functions
Function finish_init

	nsDialogs::Create /NOUNLOAD 1044;1044;1018;1044;1018
	Pop $0
	
	SetCtlColors $0 '0x000000' '0xFFFFFF'
	${NSD_CreateBitmap} 0 0 100% 100% ""
	Pop $finish_image
	${NSD_SetImage} $finish_image $PLUGINSDIR\image.png $finish_image_handle

	;${NSD_CreateLabel} 120u 10u 100% 48u "Завершение установки$\n«Рамблер-Контактов»."
	;Pop $0
	
	;SetCtlColors $0 '0x000000' '0xFFFFFF'
	
	${NSD_CreateLabel} 120u 10u 100% 60u 'Установка программы выполнена.$\nНажмите кнопку «Закрыть», чтобы$\nзавершить работу установщика.'
	Pop $0
	CreateFont $1 "$(^Font)" "13" "" 
	SendMessage $0 ${WM_SETFONT} $1 0	
	SetCtlColors $0 '0x000000' '0xFFFFFF'

	; запустить после установки
	${NSD_CreateCheckbox} 120u 80u 100% 17u "Запустить Рамблер-Контакты"
	Pop $finish_checkbox1
	SetCtlColors $finish_checkbox1 '0x000000' '0xFFFFFF'
	${NSD_Check} $finish_checkbox1
	${RUH_SetShield}

	; browser defaults
	${NSD_CreateCheckbox} 120u 100u 100% 17u "Добавить ярлык на рабочий стол"
	Pop $finish_checkbox2
	SetCtlColors $finish_checkbox2 '0x000000' '0xFFFFFF'
	${NSD_Check} $finish_checkbox2
	${RUH_SetShield}

	Call muiPageLoadFullWindow
	nsDialogs::Show
	${NSD_FreeImage} $finish_image_handle
	Call muiPageUnloadFullWindow
	Call ElemBranding
FunctionEnd

Function finish_exit
  	Pop $0

	; run program
  	${NSD_GetState} $finish_checkbox1 $1
  	${If} $1 == ${BST_CHECKED}
    	Exec '${PRODUCT_EXE}'
  	${EndIf} 
	
	; create desktop shortct	
  	${NSD_GetState} $finish_checkbox2 $2
  	${If} $2 == ${BST_CHECKED}	
		CreateShortcut "$DESKTOP\${PRODUCT_NAME}.lnk" '${PRODUCT_EXE}'
	${EndIf} 
 
FunctionEnd


; installation
Function InstallProduct
	ReadRegStr $0 HKCU "SOFTWARE\Rambler\Update\${PRODUCT_GUID}" "Version"
	ExecWait '"$LOCALAPPDATA\Rambler\RamblerUpdater\RUpdate.exe" ${PRODUCT_GUID}' $0
	
	${if} $0 = 0
		goto OnOK
	${endif}
  
	${If} $0 < 0
		MessageBox mb_iconstop "Ошибка установки $0"
		Abort
	${endif}
  
	${if} $0 = 131072035
		;MessageBox MB_ICONINFORMATION "Обновление не требуется."
	${endif}  
	
	OnOK:
		Call WriteToFireWall

	OnFinish:
		StrCpy $R9 1
		Call RelGotoPage
FunctionEnd



Function ElemBranding
  LockWindow on
  ShowWindow $mui.Branding.Background ${SW_HIDE}
  ShowWindow $mui.Branding.Text ${SW_HIDE}      
  ShowWindow $mui.Line.Standard ${SW_HIDE}
  ShowWindow $mui.Line.FullWindow ${SW_NORMAL}
  LockWindow off
FunctionEnd


Section "registry_works" aaa
  Call InstallUpdater
  Call InstallProduct
  ReadINIStr $R1 "$APPDATA\temp.ini" "Rambler" "SetSearch" 
  ${If} $R1 == 1
    ${RUH_WriteDefSearch}
  ${EndIf} 

  ReadINIStr $R1 "$APPDATA\temp.ini" "Rambler" "SetHome" 
  ${If} $R1 == 1
    ${RIS_WriteHomePage}
  ${EndIf} 
SectionEnd


Function .onInit
  InitPluginsDir
  File /oname=$PLUGINSDIR\image.png "setup.bmp"
  !insertmacro UAC_PageElevation_OnInit
  ${IfNot} ${UAC_IsInnerInstance}
    WriteINIStr $APPDATA\temp.ini "Rambler" "SetSearch" 0
	WriteINIStr $APPDATA\temp.ini "Rambler" "SetHome" 0
  ${EndIf}
  !insertmacro RUH_PageElevation_OnInit
FunctionEnd

Function .OnInstFailed
;    UAC::Unload ;Must call unload!
FunctionEnd

Function .OnInstSuccess
;    UAC::Unload ;Must call unload!
FunctionEnd

Function GuiInit
 ${IfNot} ${AtLeastWinXP}
    MessageBox MB_OK "Ваша версия Windows не поддерживается."
    Quit
  ${EndIf}  
  ${If} ${IsWinXP}
  ${AndIfNot} ${AtLeastServicePack} 1
    MessageBox MB_OK "Ваша версия Windows не поддерживается."
    Quit
  ${EndIf}  
  !insertmacro UAC_PageElevation_OnGuiInit
  !insertmacro RUH_OnGuiInit
FunctionEnd

LangString "MUI_INNERTEXT_LICENSE_TOP" 		"${LANG_${LANG}}"	""
LangString "MUI_TEXT_ABORTWARNING" 			"${LANG_${LANG}}"	"Вы действительно хотите отменить установку Рамблер-Контактов?"
LangString "MUI_TEXT_LICENSE_SUBTITLE" 		"${LANG_${LANG}}"	"Пожалуйста, прочтите его перед установкой Рамблер-Контактов."
LangString "MUI_INNERTEXT_LICENSE_BOTTOM" 	"${LANG_${LANG}}"	"Нажмите кнопку «Принимаю», если вы принимаете условия соглашения и хотите продолжить установку."
LangString "MUI_TEXT_INSTALLING_SUBTITLE"	"${LANG_${LANG}}"	"Подождите, идет копирование файлов Рамблер-Контактов."
LangString "^ShowDetailsBtn"	"${LANG_${LANG}}"	"Детали"
