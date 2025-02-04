// daemon_for_size_files_client.c
//#include "unp.h"
#include "unp_M.h"
#include <stdarg.h>
#include <syslog.h>		                                                /* for syslog() */

int daemon_proc;		                                                /* set nonzero by daemon_init() */

static void err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
	int errno_save, n;
	char buf[MAXLINE];

        //printf("errno = %d\n", errno);
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



int main(int argc, char **argv)
{
   int sockfd, n;
   char recvline[MAXLINE + 1];
   int counter_reads = 0;
   struct sockaddr_un servaddr;                                        // Структура адреса сервера.
   char sock_file[300] = {0};
   FILE *fp = NULL;
   char ch = 0;
   if(argc < 2)
       err_quit("Enter the absolute pathname of the file as the first argument to this program!");
   
   if( (fp = fopen(argv[1], "r")) == NULL)
   {
      fprintf(stderr, "Не удается открыть файл %s\n", argv[1]);
      fprintf(stderr, "Cannot open file %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   for(int i = 0; (ch = getc(fp)) != ',' && ch != ' '; i++)
   {
      sock_file[i] = ch;
   }
   
   if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)                  
       err_sys("socket error");                                        
                                                                         
   bzero(&servaddr, sizeof(servaddr));                                 
   
   servaddr.sun_family = AF_UNIX;                                         

   strncpy(servaddr.sun_path, sock_file, sizeof(servaddr.sun_path) - 1);
      
   printf("Наш клиент имеет PID = %d\n", getpid());
   printf("Our client has PID = %d\n", getpid());
   
   if(connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0)          
      err_sys("connect error");                                                                                                              
   
   while((n = read(sockfd, recvline, MAXLINE)) > 0)                    
   {                                                                   
                                                                       
      counter_reads++;                                                 
      recvline[n] = 0;           /* завершающий нуль */ /* terminating null */                          
      if(fputs(recvline, stdout) == EOF)                               
          err_sys("fputs error");
   }
   if(n < 0)
      err_sys("read error");
   printf("\ncounter_reads = %d\n", counter_reads); 
   exit(0);
}
