# The title of the installer
Name "C18"

# define the file name of the installer
outfile "c18_setup_V_1_4.exe"

# The default installation directory
 InstallDir C:\MCC18\bin

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

;--------------------------------

; The stuff to install
Section "C18 wrapper"

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put executable there
  File "c18.exe"
  
SectionEnd

Section /o "C18 Source code"
  CreateDirectory "$INSTDIR\Src"
  SetOutPath "$INSTDIR\Src"
  File "c18.vcproj"
  File "c18.suo"
  File "c18.c"
  File "getSymbolTable.c"
  File "lineLabel.c"
  File "qwiklst.c"
  File "replaceLabel.c"
  File "qwiklst.h"
  File "c18.sln"
  File "C18_Install.nsi"        
SectionEnd


Section "Create C:\Work Folder"

   CreateDirectory C:\Work
    
SectionEnd


Section "Desktop Shortcut to Work"

   # define the output path for this file by the predefined $DESKTOP variable
   setOutPath $DESKTOP

   # define what to install and place it in the output path
   file "Work.lnk"
    
SectionEnd

Section "Desktop Shortcut to DOS"

   # define the output path for this file by the predefined $DESKTOP variable
   setOutPath $DESKTOP

   # define what to install and place it in the output path
   file "DOS for work.lnk"

SectionEnd


;--------------------------------
