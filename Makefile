include Make.rules.arm

override LDFLAGS+=-rdynamic -lproc -lsatpkt -lpolydrivers -lm -ldl -lrt -lpthread
override CFLAGS+=-Wall -pedantic -std=gnu99 -g -lpthread

SRC=adcs.c
OBJS=$(SRC:.c=.o)
EXECUTABLE=adcs-sensor-reader
CMDS=adcs-sensor-reader-util
INSTALL_DEST=$(BIN_PATH)
CMD_FILE=adcs.cmd.cfg

all: $(EXECUTABLE) $(CMDS)

$(EXECUTABLE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	$(STRIP) $@

adcs-sensor-reader-util: adcs-util.c
	$(CC) $(CFLAGS) $< -lproc -lsatpkt -ldl -o $@
	$(STRIP) $@

install: $(EXECUTABLE) $(CMDS)
	cp -f $(EXECUTABLE) $(INSTALL_DEST)
	cp -f $(CMDS) $(INSTALL_DEST)
	ln -sf adcs-util $(INSTALL_DEST)/adcs-status
	ln -sf adcs-util $(INSTALL_DEST)/adcs-telemetry
	$(STRIP) $(INSTALL_DEST)/$(EXECUTABLE)
	cp $(CMD_FILE) $(ETC_PATH)

.PHONY: clean install

clean:
	rm -rf *.o $(EXECUTABLE) $(CMDS)

