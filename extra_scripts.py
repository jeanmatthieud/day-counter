# Fuses for ATTiny85 @ 1MHz
Import('env')
env.Replace(FUSESCMD="avrdude $UPLOADERFLAGS -e -Uhfuse:w:0xDF:m -Uefuse:w:0xff:m -Ulfuse:w:0x62:m")
