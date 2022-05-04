LDFLAGS=-lwiringPi
CFLAGS=-O2 -g -Wall
TARGET=backlight_switch
SERVICE=backlight_switch.service
CC=gcc
OBJS=main.o

${TARGET}:${OBJS}
	${CC} $^ -o ${TARGET} ${LDFLAGS}

%.o:%.c
	${CC} -c $^ ${CFLAGS} -o $@

all:${TARGET}

install:
	cp ${TARGET} /usr/sbin/
	cp ${SERVICE} /usr/lib/systemd/system/
	systemctl enable ${SERVICE}
	systemctl start ${SERVICE}
clean:
	rm -f *.o ${TARGET}
