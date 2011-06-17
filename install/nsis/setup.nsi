;TargetMinimalOS 5.0
!include LogicLib.nsh
!include UAC.nsh
!include WinVer.nsh
!include nsRIS.nsh
!include nsRUH.nsh

!define PRODUCT_NAME "Рамблер-Контакты"

Name "«Рамблер-Контакты»"
Caption "Рамблер-Контакты"

!define MUI_ICON "application.ico"
!define MUI_UNICON "application.ico"
!define MUI_CUSTOMFUNCTION_GUIINIT GuiInit

!define MUI_COMPONENTSPAGE_NODESC
!define MUI_PRODUCT ${PRODUCT_NAME};${PRODUCT_NAME}" 
!include "MUI2.nsh"
!include "nsDialogs.nsh"

!define MUI_WELCOMEFINISHPAGE_BITMAP "setup.bmp"
!define MUI_ABORTWARNING

!insertmacro RUH_FUNCTIONS

!ifndef BCM_SETSHIELD
!define BCM_SETSHIELD 0x0000160C
!endif

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.rtf"
!insertmacro MUI_PAGE_INSTFILES
page custom finishfull finishfullExit
!insertmacro MUI_LANGUAGE "Russian" 

RequestExecutionLevel user

!define PBS_MARQUEE 0x08

OutFile ${FILE_NAME}

XPStyle on
Icon "application.ico"

BrandingText 'ООО "Рамблер Интернет Холдинг"'

Function InstallUpdater
  SetOutPath "$TEMP"
  File "..\Holdem\Holdem.msi"
  ExecWait '"msiexec" /i "$OUTDIR\Holdem.msi" /quiet'
FunctionEnd

Var IsSetSearch_ 
Var IsSetHome_ 
Var IsRunProgram_ 

Function WriteToFireWall
  nsisFirewallW::AddAuthorizedApplication "${PRODUCT_EXE}" "${PRODUCT_NAME}"
  Pop $0
;игнорим ошибки - не вышло так не вышло 
;  IntCmp $0 0 +3
;    MessageBox MB_OK "A problem happened while adding program to Firewall exception list (result=$0)"
;    Return
;  Exec "rundll32.exe shell32.dll,Control_RunDLL firewall.cpl"
;  MessageBox MB_OK "Program added to Firewall exception list.$\r$\n(close the control panel before clicking OK)"
FunctionEnd

Function InstallProduct
  IfErrors continue1
  continue1:
  ReadRegStr $0 HKCU "SOFTWARE\Rambler\Update\${PRODUCT_GUID}" "Version"
  IfErrors OnInstallProduct 0 
  MessageBox MB_YESNO|MB_ICONQUESTION "Похоже продукт ${PRODUCT_NAME} установлен. Текщая версия $0. Выполнить обновление?" IDNO OnFinish 
;  WriteRegStr HKLM "SOFTWARE\Rambler\Update\${PRODUCT_GUID}" "Version" "0.0.0.0"  
  OnInstallProduct:
  IfErrors continue
  continue:
  ExecWait '"$LOCALAPPDATA\Rambler\RamblerUpdater\RUpdate.exe" ${PRODUCT_GUID}' $0
  IfErrors 0 OnCont
    MessageBox mb_iconstop "Не удалось запустить установить ядро установщика."
    Abort
  OnCont:
;  MessageBox mb_iconstop "Результат установки $0"
  ${if} $0 = 0
    goto OnOK
  ${endif}
  ${If} $0 < 0
;  IfErrors 0 OnOK
    MessageBox mb_iconstop "Ошибка установки $0"
    Abort
  ${endif}
  ${if} $0 = 131072035
    MessageBox MB_ICONINFORMATION "Обновление не требуется."
  ${endif}
  OnOK:
  Call WriteToFireWall
;  WriteRegStr HKLM "Software\Microsoft\Internet Explorer\Toolbar" "{D276614E-4A9C-4A98-819A-C8922CA9756C}" "0"
;  StrCmp ${PRODUCT_EXE} "void" OnFinish
;  MessageBox MB_YESNO|MB_ICONQUESTION "Запустить ${PRODUCT_NAME}?" IDNO OnFinish
;  Exec '${PRODUCT_EXE}'
  OnFinish:
FunctionEnd

!insertmacro MUI_PAGE_FUNCTION_FULLWINDOW


Var Checkbox1
Var Checkbox2
Var Checkbox3
Var Image
Var ImageHandle

Function finishfull
  nsDialogs::Create /NOUNLOAD 1044;1044;1018;1044;1018
  Pop $0
  SetCtlColors $0 '0x000000' '0xFFFFFF'
  ${NSD_CreateBitmap} 0 0 100% 100% ""
  Pop $Image
  ${NSD_SetImage} $Image $PLUGINSDIR\image.png $ImageHandle

  ${NSD_CreateLabel} 120u 10u 100% 24u "Установка ${PRODUCT_NAME}"
  Pop $0
  SetCtlColors $0 '0x000000' '0xFFFFFF'
  CreateFont $1 "$(^Font)" "15" "" 
  SendMessage $0 ${WM_SETFONT} $1 0
  ${NSD_CreateLabel} 120u 36u 100% 24u 'Для установки Рамблер-Контактов на ваш компьютер $\r$\nнажмите кнопку «Установить».'
  Pop $0
  SetCtlColors $0 '0x000000' '0xFFFFFF'
    
  ${NSD_OnBack} RUH_RemShield

  ; запустить после установки
  ${NSD_CreateCheckbox} 120u 70u 100% 17u "Запустить Рамблер-Контакты"
  Pop $Checkbox1
  SetCtlColors $Checkbox1 '0x000000' '0xFFFFFF'
  ${NSD_Check} $Checkbox1
  ${RUH_SetShield}
  
  ; browser defaults
  ${NSD_CreateCheckbox} 120u 90u 100% 17u "Сделать Рамблер поиском по-умолчанию"
  Pop $Checkbox2
  SetCtlColors $Checkbox2 '0x000000' '0xFFFFFF'
  ${NSD_Check} $Checkbox2
  ${NSD_OnClick} $Checkbox2 OnClickCheckAdminRights
  ${RUH_SetShield}

  ; browser defaults
  ${NSD_CreateCheckbox} 120u 110u 100% 17u "Сделать Рамблер главной страницей"
  Pop $Checkbox3
  SetCtlColors $Checkbox3 '0x000000' '0xFFFFFF'
  ${NSD_Check} $Checkbox3
  ${NSD_OnClick} $Checkbox3 OnClickCheckAdminRights
  ${RUH_SetShield}

  Call muiPageLoadFullWindow
  nsDialogs::Show
  ${NSD_FreeImage} $ImageHandle
  Call muiPageUnloadFullWindow
  Call ElemBranding
FunctionEnd

Function OnClickCheckAdminRights
  Pop $0 ; don't forget to pop HWND of the stack
  ${NSD_GetState} $Checkbox2 $1
  ${NSD_GetState} $Checkbox3 $2

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

Function finishfullExit
  	Pop $0

	${NSD_GetState} $Checkbox2 $IsSetSearch_
  	${If} $IsSetSearch_ == ${BST_CHECKED}
    	WriteINIStr $APPDATA\temp.ini "Rambler" "SetSearch" 1
    	call RUH_SetAdminIfNeeded 
  	${EndIf}

  	${NSD_GetState} $Checkbox3 $IsSetHome_
  	${If} $IsSetHome_ == ${BST_CHECKED}
    	WriteINIStr $APPDATA\temp.ini "Rambler" "SetHome" 1
    	call RUH_SetAdminIfNeeded 
  	${EndIf}

	; run program
  	${NSD_GetState} $Checkbox1 $IsRunProgram_
  	${If} $IsRunProgram_ == ${BST_CHECKED}
    	Exec '${PRODUCT_EXE}'
  	${EndIf} 
 
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

;TODO
;;;;;  ReadRegStr $0 HKLM "SOFTWARE\Rambler\Update\${PRODUCT_GUID}" "Version"
;;;;;  IfErrors OnInstallProduct 0 
;;;;;  MessageBox MB_YESNO|MB_ICONQUESTION "Похоже продукт ${PRODUCT_NAME} установлен. Текщая версия $0. Выполнить обновление?" IDNO OnFinish 
;;;;;;  WriteRegStr HKLM "SOFTWARE\Rambler\Update\${PRODUCT_GUID}" "Version" "0.0.0.0"  
;;;;;  OnInstallProduct:
;;;;;  IfErrors continue
;;;;;  continue:
;;;;;  ExecWait '"$PROGRAMFILES\Rambler\RamblerUpdater\RUpdate.exe" ${PRODUCT_GUID}' $0
;;;;;  ${if} $0 == "0"
;;;;;    goto OnOK
;;;;;  ${endif}
;;;;;  IfErrors 0 OnOK
;;;;;    MessageBox mb_iconstop "Ошибка установки $0"
;;;;;    Abort
;;;;;  OnOK:
;;;;;  StrCmp ${PRODUCT_EXE} "void" OnFinish
;;;;;  MessageBox MB_YESNO|MB_ICONQUESTION "Запустить ${PRODUCT_NAME}?" IDNO OnFinish
;;;;;  Exec '${PRODUCT_EXE}'
;;;;;  OnFinish:
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


