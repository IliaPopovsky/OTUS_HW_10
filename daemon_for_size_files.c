#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    FILE *fp = NULL;
    long last = 0;

    if((fp = fopen("control_file.txt", "rb")) == NULL)
    {
       printf("Not open file %s\n", "control_file.txt");
       exit(EXIT_FAILURE);
    }
    fseek(fp, 0L, SEEK_END);               // перейти в конец файла
    last = ftell(fp);
    printf("last = %ld\n", last);
    printf("Hello world!\n");
    return 0;
}
