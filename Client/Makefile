include ./Make.defines

PROGS =	daemon_for_size_files_client

all:	${PROGS}

daemon_for_size_files_client:	daemon_for_size_files_client.o
		${CC} ${CFLAGS} -o $@ daemon_for_size_files_client.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
