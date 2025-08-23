include Make.rules.arm

override LDFLAGS+=-rdynamic -lproc -lsatpkt -lpolydrivers -lm -ldl -lrt -lpthread -Lpolypkt -l$(PKT_LIB_NAME)
override CFLAGS+=-Wall -pedantic -std=gnu99 -g -lpthread

SRC=adcs.c
OBJS=$(SRC:.c=.o)
EXECUTABLE=adcs-sensor-reader
CMDS=adcs-sensor-reader-util
INSTALL_DEST=$(BIN_PATH)
XDRGEN?=poly-xdrgen

# Packet Library Build Variables
PKT_LIB_NAME=adcs-sensor-reader-pkt
PKT_LIB_DIR=$(shell pwd)/polypkt
PKT_LIB_SCHEMA_SRC=adcs_reader_schema.xp
PKT_LIB_C_SRC=$(PKT_LIB_DIR)/adcs-sensor-reader-pkt
PKT_LIB_MAJOR_VERS=1
PKT_LIB_MINOR_VERS=0
PKT_LIB_SO_NAME=lib$(PKT_LIB_NAME).$(SO_EXT)
PKT_LIB_SO_NAME_VERS=$(LIBRARY).$(SO_EXT).$(MAJOR_VERS).$(MINOR_VERS)
PKT_LIB=$(PKT_LIB_DIR)/$(PKT_LIB_SO_NAME)
override PKT_LIB_LDFLAGS+=-lproc -shared -fPIC -Wl,-soname,$(PKT_LIB_SO_NAME)

all: $(EXECUTABLE) $(CMDS)

$(EXECUTABLE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	$(STRIP) $@

adcs-sensor-reader-util: adcs-util.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
	$(STRIP) $@

adcs.o: adcs.c $(PKT_LIB)

$(PKT_LIB_C_SRC).c: $(PKT_LIB_SCHEMA_SRC) | $(PKT_LIB_DIR)
	$(XDRGEN) --target libproc --output $(PKT_LIB_C_SRC) $(PKT_LIB_SCHEMA_SRC)

$(PKT_LIB_DIR):
	mkdir -p $(PKT_LIB_DIR)

$(PKT_LIB): $(PKT_LIB_C_SRC).c
	$(CC) $(PKT_LIB_C_SRC).c $(SO_LDFLAGS) $(PKT_LIB_LDFLAGS) $(CFLAGS) -o $@

install: $(EXECUTABLE) install-polypkt $(CMDS)
	cp -f $(EXECUTABLE) $(INSTALL_DEST)
	cp -f $(CMDS) $(INSTALL_DEST)
	ln -sf adcs-sensor-reader-util $(INSTALL_DEST)/adcs-status
	$(STRIP) $(INSTALL_DEST)/$(EXECUTABLE)

install-polypkt: $(PKT_LIB)
	install -d $(INC_PATH)/polypkt
	cp polypkt/adcs-sensor-reader-pkt.h $(INC_PATH)/polypkt
	cp $(PKT_LIB) $(LIB_PATH)/$(LIB_NAME)
	ln -sf $(LIB_NAME) $(LIB_PATH)/$(LIBRARY).$(SO_EXT)
	ln -sf $(LIB_NAME) $(LIB_PATH)/$(LIBRARY).$(SO_EXT).$(MAJOR_VERS)


.PHONY: clean install

clean:
	rm -rf *.o $(EXECUTABLE) $(CMDS) polypkt

