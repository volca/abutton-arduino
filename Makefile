flash:
	platformio run -t upload 

clean:
	platformio run -t clean 

monitor:
	picocom -b 115200 /dev/tty.SLAB_USBtoUART
