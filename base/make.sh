rm ./cswitch.o
rm ./util.o
rm ./os.o
rm ./os.hex
rm ./os.elf
rm lcd/lcd_drv.o
rm base_station.o
rm remote.o
rm uart.o
rm roomba.o


avr-gcc -Wall -Os -DF_CPU=16000000 -mmcu=atmega2560 -c lcd/lcd_drv.c -o lcd/lcd_drv.o
avr-gcc -Wall -Os -DF_CPU=16000000 -mmcu=atmega2560 -c os.c -o os.o
avr-gcc -Wall -Os -DF_CPU=16000000 -mmcu=atmega2560 -c uart/uart.c -o uart.o
avr-gcc -Wall -Os -DF_CPU=16000000 -mmcu=atmega2560 -c base_station.c -o base_station.o
avr-gcc -c -O2 -mmcu=atmega2560 -Wa,--gstabs -o cswitch.o cswitch.s
avr-gcc -mmcu=atmega2560  uart.o os.o base_station.o cswitch.o lcd/lcd_drv.o -o base_station.elf
avr-objcopy -O ihex -R .eeprom base_station.elf base_station.hex
avrdude -v -p atmega2560 -c wiring -P /dev/cu.usbmodem1431 -b 115200 -D -U flash:w:base_station.hex:i
