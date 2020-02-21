parse_dump:
	java -jar EspStackTraceDecoder.jar ~/.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-addr2line  .pio/build/esp07/firmware.elf dump.txt