// daemon_for_size_files_server.c
#include "unp.h"
//#include "unp_M.h"

#include <stdarg.h>
#include <syslog.h>		                                                /* for syslog() */

#include <sys/resource.h>

int daemon_proc;		                                                /* set nonzero by daemon_init() */

static void err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
	int errno_save, n;
	char buf[MAXLINE];

	errno_save = errno;		                                        /* value caller might want printed */
#ifdef	HAVE_VSNPRINTF
	vsnprintf(buf, sizeof(buf), fmt, ap);	                                 /* this is safe */
#else
	vsprintf(buf, fmt, ap);					          /* this is not safe */
#endif
	n = strlen(buf);
	if (errnoflag)
		snprintf(buf+n, sizeof(buf)-n, ": %s", strerror(errno_save));
	strcat(buf, "\n");

	if (daemon_proc) {
		syslog(level, buf, "empty");
	} else {
		fflush(stdout);		                                 /* in case stdout and stderr are the same */
		fputs(buf, stderr);
		fflush(stderr);
	}
	return;
}

void err_quit(const char *fmt, ...)
{
	va_list ap;
        va_start(ap, fmt);
	err_doit(0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

void err_sys(const char *fmt, ...)
{
	va_list ap;
        va_start(ap, fmt);
	err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}


int Socket(int family, int type, int protocol)                                 // Наша собственная функция-обертка для функции socket.
{                                                                              // Our own wrapper function for the socket function.
  int n;
  if( (n = socket(family, type, protocol)) < 0)
       err_sys("socket error");
  return (n);
}

void Bind(int fd, const struct sockaddr *sa, socklen_t salen)                  // Наша собственная функция-обертка для функции bind.
{                                                                              // Our own wrapper function for the bind function.
	if (bind(fd, sa, salen) < 0)
		err_sys("bind error");
}

void Listen(int fd, int backlog)                                               // Наша собственная функция-обертка для функции listen.
{                                                                              // Our own wrapper function for the listen function.
	char	*ptr;

		/*4can override 2nd argument with environment variable */
	if ( (ptr = getenv("LISTENQ")) != NULL)
		backlog = atoi(ptr);

	if (listen(fd, backlog) < 0)
		err_sys("listen error");
}

void Write(int fd, void *ptr, size_t nbytes)                                   // Наша собственная функция-обертка для функции write.
{                                                                              // Our own wrapper function for the write function.
	if ((size_t)write(fd, ptr, nbytes) != nbytes)
		err_sys("write error");
}

void Close(int fd)                                                             // Наша собственная функция-обертка для функции close.
{                                                                              // Our own wrapper function for the close function.
	if (close(fd) == -1)
		err_sys("close error");
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)                   // Наша собственная функция-обертка для функции accept. 
{                                                                              // Our own wrapper function for the accept function.
	int		n;

again:
	if ( (n = accept(fd, sa, salenptr)) < 0) {
#ifdef	EPROTO
		if (errno == EPROTO || errno == ECONNABORTED)
#else
		if (errno == ECONNABORTED)
#endif
			goto again;
		else
			err_sys("accept error");
	}
	return(n);
}

ssize_t writen(int fd, const void *vptr, size_t n)					/* Write "n" bytes to a descriptor. */
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

void Writen(int fd, void *ptr, size_t nbytes)
{
	if ((size_t)writen(fd, ptr, nbytes) != nbytes)
		err_sys("writen error");
}

void responce_size_file(int sockfd, long size)
{
   ssize_t n;
   char buf[MAXLINE];
   memset(buf, 0, MAXLINE);
   snprintf(buf, sizeof(buf), "%ld\n", size);
   n = strlen(buf); 
   Writen(sockfd, buf, n); 
   printf("Ответ сервера отправлен!\n"); 
   printf("Server response sent!\n");
   return;
}

void mistake_open_file_for_clients(int sockfd, char *array)
{
   ssize_t n;
   char buf[MAXLINE];
   memset(buf, 0, MAXLINE);
   snprintf(buf, sizeof(buf), "Не удается открыть файл (Cannot open file) %s\n", array);
   n = strlen(buf); 
   Writen(sockfd, buf, n); 
   printf("Ответ сервера отправлен!\n"); 
   printf("Server response sent!\n");
   return;
   
}

void sig_chld(int signo)
{
	pid_t pid;
	int stat = signo;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return;
}

Sigfunc *signal(int signo, Sigfunc *func)
{
	struct sigaction	act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
#endif
	} else {
#ifdef	SA_RESTART
		act.sa_flags |= SA_RESTART;		/* SVR4, 44BSD */
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return(SIG_ERR);
	return(oact.sa_handler);
}
/* end signal */

Sigfunc *Signal(int signo, Sigfunc *func)	/* for our signal() function */
{
	Sigfunc	*sigfunc;

	if ( (sigfunc = signal(signo, func)) == SIG_ERR)
		err_sys("signal error");
	return(sigfunc);
}



void daemonize(const char* cmd);

int main(int argc, char **argv)
{
   int listenfd, connfd;
   pid_t childpid;
   socklen_t clilen;
   struct sockaddr_un cliaddr, servaddr;
   char sock_file[300] = {0};
   char name_file[300] = {0};
   long size = 0;
   FILE *fp = NULL;
   FILE *fpt = NULL;
   char ch = 0;
   char control_symbol = 0;
   char *cmd;
   
   if(argc < 2)
       err_quit("Enter the absolute pathname of the file as the first argument to this program!");
   if ((cmd = strrchr(argv[0], '/')) == NULL)
       cmd = argv[0];
   else
       cmd++;
   printf("cmd = %s\n", cmd);
   printf("Вы хотите демонизировать (перевести в фоновый режим работы) наш процесс с PID = %d?\n", getpid());
   printf("You want to daemonize (put into the background) our process with PID = %d?\n", getpid());
   printf("Введите y если ДА, введите n (или любой другой символ отличный от y) если НЕТ:\n");
   printf("Enter y if YES, enter n (or any other character other than y) if NO:\n");
   scanf("%c", &control_symbol);
   while(getchar() != '\n')
       continue;

   if(control_symbol == 'y')
   {
       daemonize(cmd);
   }
   
   if( (fp = fopen(argv[1], "r")) == NULL)
   {
      fprintf(stderr, "Не удается открыть файл %s\n", argv[1]);
      fprintf(stderr, "Unable to open file %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   for(int i = 0; (ch = getc(fp)) != ',' && ch != ' '; i++)
   {
      sock_file[i] = ch;
   }
   for(int i = 0; (ch = getc(fp)) != EOF;)
   {
      if(ch != ' ' && ch != ',' && ch != '\n')
      {
         name_file[i] = ch;
         i++;
      }
   }
   printf("name_file = $%s$\n", name_file);
   
   unlink(sock_file);
   
   listenfd = Socket(AF_UNIX, SOCK_STREAM, 0);                                                           
   bzero(&servaddr, sizeof(servaddr));                                 
   servaddr.sun_family = AF_UNIX;                                      
   strncpy(servaddr.sun_path, sock_file, sizeof(servaddr.sun_path) - 1);
   
   Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));                  
                                                                       
   Listen(listenfd, LISTENQ);                                          
   
   Signal(SIGCHLD, sig_chld);           /* нужно вызвать waitpid() */       /* need to call waitpid() */                                                             
   printf("Наш сервер работает!\n");
   printf("Our server is working!\n");
   printf("Наш сервер имеет PID = %d\n", getpid());
   printf("Our server has a PID = %d\n", getpid());
   
   printf("Наш сервер имеет прослушиваемый сокет listenfd = %d\n", listenfd);
   printf("Our server has a listening socket listenfd = %d\n", listenfd);
   
                                                                   
   for(;;){
      printf("Цикл нашего сервера работает! (До вызова accept())\n");
      printf("Our server loop is running! (Before calling accept())\n");
      clilen = sizeof(cliaddr); 
      if( (connfd = Accept(listenfd, (SA *) &cliaddr, &clilen)) < 0){  
         if(errno == EINTR)
             continue;                      /* назад к for() */  /* back to for() */
         else
             err_sys("accept error");
     }
     printf("Наш сервер имеет присоединенный сокет connfd = %d\n", connfd);   
     printf("Our server has a connfd socket attached = %d\n", connfd);                                                               
     
     
     if( (childpid = fork()) == 0){  /* дочерний процесс */  /* child process */
       printf("Данный дочерний процесс имеет PID = %d\n", getpid());
       printf("This child process has a PID = %d\n", getpid());
       
       Close(listenfd);             /* закрываем прослушиваемый сокет */  /* close the listening socket */
       if( (fpt = fopen(name_file, "r")) == NULL)
       {
          mistake_open_file_for_clients(connfd, name_file);
          printf("Не удалось открыть файл (сообщение для сервера) %s\n", name_file);
          printf("Failed to open file (message to server) %s\n", name_file);
          exit(EXIT_FAILURE);
       }
       fseek(fpt, 0L, SEEK_SET);               // перейти в начало файла    // go to the beginning of the file
       fseek(fpt, 0L, SEEK_END);               // перейти в конец файла     // go to the end of the file
       size = ftell(fpt);
       responce_size_file(connfd, size);            /* обрабатываем запрос */  /* process the request */
       exit(0);
     }
     Close(connfd);                 /* родительский процесс закрывает присоединенный сокет */  /* parent process closes attached socket */
   }
}

void daemonize(const char* cmd)
{
           /*
            * Инициализировать файл журнала. Initialize the log file.
            */
           openlog(cmd, LOG_CONS, LOG_DAEMON);
           /*
            * Сбросить маску режима создания файла. Reset file creation mode mask.
            */
           umask(0);
           /*
            * Получить максимально возможный номер дескриптора файла. Get the highest possible file descriptor number.
            */
           struct rlimit rl;
           if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
               perror("невозможно получить максимальный номер дескриптора (unable to get maximum descriptor number)");
           /*
            * Стать лидером нового сеанса, чтобы утратить управляющий терминал. Become the leader of a new session to lose the control terminal.
            */
           pid_t pid;
           if ((pid = fork()) < 0)
               perror("ошибка вызова функции fork (error calling fork function)");
           else if (pid != 0) /* родительский процесс */ /* parent process */
               exit(0);
           setsid();
           /*
            * Обеспечить невозможность обретения управляющего терминала в будущем. Ensure that it is impossible to acquire a control terminal in the future.
            */
           struct sigaction sa;
           sa.sa_handler = SIG_IGN;
           sigemptyset(&sa.sa_mask);
           sa.sa_flags = 0;
           if (sigaction(SIGHUP, &sa, NULL) < 0)
               syslog(LOG_CRIT, "невозможно игнорировать сигнал SIGHUP (it is impossible to ignore the SIGHUP signal)");
           if ((pid = fork()) < 0)
               syslog(LOG_CRIT, "ошибка вызова функции fork (error calling fork function)");
           else if (pid != 0) /* родительский процесс */  /* parent process */
               exit(0);
           /*
            * Назначить корневой каталог текущим рабочим каталогом,
            * чтобы впоследствии можно было отмонтировать файловую систему. 
            * Set the root directory as the current working directory,
            * so that the file system can be unmounted later.
            */
           if (chdir("/") < 0)
               syslog(LOG_CRIT, "невозможно сделать текущим рабочим каталогом / (cannot make current working directory /)");
           /*
            * Закрыть все открытые файловые дескрипторы. Close all open file descriptors.
            */
           if (rl.rlim_max == RLIM_INFINITY)
               rl.rlim_max = 1024;
           for (int i = 0; i < (int)rl.rlim_max; i++)
               close(i);

            /*
             * Присоединить файловые дескрипторы 0, 1 и 2 к /dev/null. Attach file descriptors 0, 1, and 2 to /dev/null.
             */
           int fd0 = open("/dev/null", O_RDWR);
           int fd1 = dup(0);
           int fd2 = dup(0);
           if (fd0 != 0 || fd1 != 1 || fd2 != 2)
           {
              syslog(LOG_CRIT, "ошибочные файловые дескрипторы %d %d %d",fd0, fd1, fd2);
              syslog(LOG_CRIT, "bad file descriptors %d %d %d",fd0, fd1, fd2);
              exit(1);
           }

} 
 
