SET VER_OLD=1.5.0.780
SET VER_NEW=0.2.0.868

mkdir output\%VER_OLD%_to_%VER_NEW%
mkdir output\%VER_OLD%_to_%VER_NEW%/tmp

torch -p -xi .\output\%VER_OLD%\contacts.wixpdb .\output\%VER_NEW%\contacts.wixpdb  -out .\output\%VER_OLD%_to_%VER_NEW%\diff.wixmst

candle.exe -out output/%VER_OLD%_to_%VER_NEW%/patch.wixobj wxs/patch.wxs
light.exe output/%VER_OLD%_to_%VER_NEW%/patch.wixobj -out output\%VER_OLD%_to_%VER_NEW%\patch.wixmsp
pyro.exe ./output/%VER_OLD%_to_%VER_NEW%/patch.wixmsp -out output\%VER_OLD%_to_%VER_NEW%\patch.msp -t RTM ./output/%VER_OLD%_to_%VER_NEW%/diff.wixmst

