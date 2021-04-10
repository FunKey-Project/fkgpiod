TOOLS_CFLAGS	:= -Wall -std=c99 -D _DEFAULT_SOURCE
#
# Programs
#
all: fkgpiod termfix

fkgpiod: main.o daemon.o parse_config.o mapping_list.o gpio_mapping.o gpio_utils.o gpio_axp209.o gpio_pcal6416a.o smbus.o uinput.o keydefs.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

termfix: termfix.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

#
# Objects
#

%.o: %.c
	$(CC) $(CFLAGS) $(TOOLS_CFLAGS) -c $< -o $@

clean:
	rm -f *.o fkgpiod termfix
