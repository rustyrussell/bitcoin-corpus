CCAN_DIR:=../ccan
CFLAGS = -g -Wall -I$(CCAN_DIR)
CCAN_CFILES := $(CCAN_DIR)/ccan/err/err.c $(CCAN_DIR)/ccan/htable/htable.c $(CCAN_DIR)/ccan/likely/likely.c $(CCAN_DIR)/ccan/list/list.c $(CCAN_DIR)/ccan/noerr/noerr.c $(CCAN_DIR)/ccan/rbuf/rbuf.c $(CCAN_DIR)/ccan/read_write_all/read_write_all.c $(CCAN_DIR)/ccan/str/debug.c $(CCAN_DIR)/ccan/str/str.c $(CCAN_DIR)/ccan/take/take.c $(CCAN_DIR)/ccan/tal/tal.c $(CCAN_DIR)/ccan/tal/grab_file/grab_file.c
CCANOBJS := $(CCAN_CFILES:.c=.o)

default: encode-dump

encode-dump: $(CCANOBJS) encode-dump.o

clean:
	$(RM) encode-dump *.o $(CCANOBJS)
