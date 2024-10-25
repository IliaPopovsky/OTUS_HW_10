#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define NAME "echo.socket"

#include <string.h>
#include <errno.h>

int main(void)
{
    FILE *fp = NULL;
    FILE *fpt = NULL;
    long last = 0;
    char name_file[100] = {0};
    char name_socket[100] = {0};
    int ch = 0;
    char control_symbol = 0;
    pid_t ret;
    printf("Вы хотите демонизировать (перевести в фоновый режим работы) наш процесс с PID = %d?\n", getpid());
    printf("Введите d если ДА, введите n (или любой другой символ отличный от d) если НЕТ:\n");
    scanf("%c", &control_symbol);
    while(getchar() != '\n')
        continue;

    if(control_symbol == 'd')
    {
        void daemonize(const char* cmd)
        {
           /*
            * Инициализировать файл журнала.
            */
           openlog(cmd, LOG_CONS, LOG_DAEMON);
           /*
            * Сбросить маску режима создания файла.
            */
           umask(0);
           /*
            * Получить максимально возможный номер дескриптора файла.
            */
           struct rlimit rl;
           if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
               perror("невозможно получить максимальный номер дескриптора");
           /*
            * Стать лидером нового сеанса, чтобы утратить управляющий терминал.
            */
           pid_t pid;
           if ((pid = fork()) < 0)
               perror("ошибка вызова функции fork");
           else if (pid != 0) /* родительский процесс */
               exit(0);
           setsid();
           /*
            * Обеспечить невозможность обретения управляющего терминала в будущем.
            */
           struct sigaction sa;
           sa.sa_handler = SIG_IGN;
           sigemptyset(&sa.sa_mask);
           sa.sa_flags = 0;
           if (sigaction(SIGHUP, &sa, NULL) < 0)
               syslog(LOG_CRIT, "невозможно игнорировать сигнал SIGHUP");
           if ((pid = fork()) < 0)
               syslog(LOG_CRIT, "ошибка вызова функции fork");
           else if (pid != 0) /* родительский процесс */
               exit(0);
           /*
            * Назначить корневой каталог текущим рабочим каталогом,
            * чтобы впоследствии можно было отмонтировать файловую систему.
            */
           if (chdir("/") < 0)
               syslog(LOG_CRIT, "невозможно сделать текущим рабочим каталогом /");
           /*
            * Закрыть все открытые файловые дескрипторы.
            */
           if (rl.rlim_max == RLIM_INFINITY)
               rl.rlim_max = 1024;
           for (int i = 0; i < rl.rlim_max; i++)
               close(i);

            /*
             * Присоединить файловые дескрипторы 0, 1 и 2 к /dev/null.
             */
           int fd0 = open("/dev/null", O_RDWR);
           int fd1 = dup(0);
           int fd2 = dup(0);
           if (fd0 != 0 || fd1 != 1 || fd2 != 2)
           syslog(LOG_CRIT, "ошибочные файловые дескрипторы %d %d %d",fd0, fd1, fd2);
        }
    }

    if((fp = fopen("my_daemon.conf", "rb")) == NULL)
    {
       printf("Not open file %s\n", "my_daemon.conf");
       exit(EXIT_FAILURE);
    }
    for(int i = 0; (ch = getc(fp)) != ','; i++)
    {
       if(('A' <= ch && ch <='Z') || ('a' <= ch && ch <='z') || ch == '.' || ch == '_')
           name_file[i] = ch;
       else
           i--;
    }
    for(int i = 0; (ch = getc(fp)) != EOF; i++)
    {
       if(('A' <= ch && ch <='Z') || ('a' <= ch && ch <='z') || ch == '.' || ch == '_')
           name_socket[i] = ch;
       else
           i--;
    }

    if((fpt = fopen(name_file, "rb")) == NULL)
    {
       printf("Not open file %s\n", name_file);
       exit(EXIT_FAILURE);
    }
    fseek(fpt, 0L, SEEK_END);               // перейти в конец файла
    last = ftell(fpt);
    printf("last = %ld\n", last);
    #if 1
    unlink((name_socket));
    ret = fork();
    if(ret)
    {
       #if 1                          // Тоже рабочая схема.
       int sock, msgsock, rval;
       struct sockaddr_un server;
       char buf[1024];
       sock = socket(AF_UNIX, SOCK_STREAM, 0);
       if (sock < 0) {
          perror("opening stream socket");
          exit(1);
       }
       server.sun_family = AF_UNIX;
       //strcpy(server.sun_path, NAME);
       strcpy(server.sun_path, name_socket);
       if (bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) {
          perror("binding stream socket");
          exit(1);
       }
       printf("Socket has name %s\n", server.sun_path);
       listen(sock, 5);
       for (;;) {
          msgsock = accept(sock, 0, 0);
       if (msgsock == -1)
       {
          perror("accept");
          break;
       }
       else do {
                bzero(buf, sizeof(buf));
                if ((rval = read(msgsock, buf, 1024)) < 0)
                    perror("reading stream message");
                else if (rval == 0)
                         printf("Ending connection\n");
                     else
                         send(msgsock, buf, strlen(buf), 0);
               } while (rval > 0);
       close(msgsock);
       }
       close(sock);
       //unlink(NAME);
       unlink(name_socket);
       #endif // 0
       //sleep(60);
       //system("nc -U echo.socket < /home/ilia/OTUS_HW_10/control_file");
       //system("/home/ilia/Socket_UDS/echo_socket.out");              // Рабочая схема.

    }
    else
    {
        //execvp("bash");
       //execvp("/home/ilia/Socket_UDS/echo_socket.out", NULL);
       //system("/home/ilia/Socket_UDS/echo_socket.out");
       sleep(2);
       //char *args[] = {"nc", "-U", name_socket, "< /home/ilia/OTUS_HW_10/control_file"};
       //char *args[] = {"nc", "echo.socket",  "<", "/home/ilia/OTUS_HW_10/control_file", 0};
       //execvp("nc", args);
       //printf("execvp() failed. Error: %s\n", strerror(errno));
       char target_string[100] = {0};
       //sprintf(target_string, "nc -U %s < /home/ilia/OTUS_HW_10/control_file", name_socket);  // Рабочая схема.
       sprintf(target_string, "echo %ld | nc -U %s", last, name_socket);
       //char *target_string = "nc -U echo.socket < /home/ilia/OTUS_HW_10/control_file";   // Рабочая схема.
       system(target_string);
       //system("nc -U echo.socket < /home/ilia/OTUS_HW_10/control_file");        // Рабочая схема.
       //system("nc -U -s name_socket < /home/ilia/OTUS_HW_10/control_file");

    }
    #endif // 0
    #if 0
    system("/home/ilia/Socket_UDS/echo_socket.out");
    sleep(10);
    system("nc -U echo.socket < /home/ilia/OTUS_HW_10/control_file");   // НЕ работающая схема.
    #endif
    printf("Hello world!\n");
    return 0;
}
