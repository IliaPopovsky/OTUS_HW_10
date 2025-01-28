include ./Make.defines

PROGS =	daemon_for_size_files_server

all:	${PROGS}

daemon_for_size_files_server:	daemon_for_size_files_server.o
		${CC} ${CFLAGS} -o $@ daemon_for_size_files_server.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
