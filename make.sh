rm ./cswitch.o
rm ./util.o
rm ./os.o
rm ./os.hex
rm ./os.elf
rm lcd/lcd_drv.o

avr-gcc -Wall -Os -DF_CPU=16000000 -mmcu=atmega2560 -c lcd/lcd_drv.c -o lcd/lcd_drv.o
avr-gcc -c -O2 -mmcu=atmega2560 -Wa,--gstabs -o cswitch.o cswitch.s
avr-gcc -Wall -Os -DF_CPU=16000000 -mmcu=atmega2560 -c os.c -o os.o
avr-gcc -mmcu=atmega2560 os.o cswitch.o lcd/lcd_drv.o -o os.elf
avr-objcopy -O ihex -R .eeprom os.elf os.hex
avrdude -v -p atmega2560 -c wiring -P /dev/cu.usbmodem1411 -b 115200 -D -U flash:w:os.hex:i