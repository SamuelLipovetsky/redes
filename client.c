#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 4000
void usage(int argc, char **argv)
{
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}



int divideString(const char *str, char **remainingString)
{
    const char *selectFilePrefix = "select file";
    const char *sendFilePrefix = "send file";

    if (strncmp(str, selectFilePrefix, strlen(selectFilePrefix)) == 0)
    {

        *remainingString = strdup(str + strlen(selectFilePrefix) + 1);

        size_t length = strlen(*remainingString);
        if (length > 0 && (*remainingString)[length - 1] == '\n')
        {
            (*remainingString)[length - 1] = '\0';
        }
        return 1;
    }

    else if (strncmp(str, sendFilePrefix, strlen(sendFilePrefix)) == 0)
    {

        return 0;
    }

    return -1;
}
int endsWithValidExtension(const char *str)
{
    const char *validExtensions[] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};
    int numValidExtensions = 6;

    int length = strlen(str);

    if (length >= 4)
    {
        // const char* lastFourChars = str + length - 4;
        const char *extension = strrchr(str, '.');

        for (int i = 0; i < numValidExtensions; i++)
        {
            if (strcmp(extension, validExtensions[i]) == 0)
            {
                return 1; // True
            }
        }
    }

    return 0; // False
}
#include <stdio.h>
#include <stdlib.h>

void buf_file_range(FILE *file, long start, long end, char *buffer, long file_size, char *file_name)
{
    
    if (start <= 0)
    {
        strcpy(buffer, file_name);
        fseek(file, start, SEEK_SET);
        long length = end - start;
        fread(buffer + strlen(file_name), sizeof(char), length - strlen(file_name), file);
    }
    else if (end >= file_size)
    {
        fseek(file, start, SEEK_SET);
        long length = file_size - start;
        fread(buffer, sizeof(char), length, file);
        strcat(buffer,"/end");
       
    }
    else
    {
        long length = end - start;
        fseek(file, start, SEEK_SET);

        fread(buffer, sizeof(char), length, file);
    }
   
}
long get_file_size(FILE *file)
{
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argc, argv);
    }

    int file_selected = 0;
    char *file_name;
    while (1)
    {
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);

        fgets(buf, BUFSZ - 1, stdin);

        int command_type = divideString(buf, &file_name);
        // select file

        if (command_type == 1)
        {
            if (!endsWithValidExtension(file_name))
            {
                file_selected = 0;
                printf("%s not valid! \n", file_name);
            }
            else if (access(file_name, F_OK) == -1)
            {
                file_selected = 0;
                printf("%s does not exist \n", file_name);
            }
            else
            {
                file_selected = 1;
                printf("%s selected \n", file_name);
            }
        }
        else if (command_type == 0)
        {

            struct sockaddr_storage storage;
            if (0 != addrparse(argv[1], argv[2], &storage))
            {
                usage(argc, argv);
            }

            int s;
            s = socket(storage.ss_family, SOCK_STREAM, 0);
            if (s == -1)
            {
                logexit("socket");
            }
            struct sockaddr *addr = (struct sockaddr *)(&storage);
            if (0 != connect(s, addr, sizeof(storage)))
            {
                logexit("connect");
            }

            char addrstr[BUFSZ];
            addrtostr(addr, addrstr, BUFSZ);

            if (file_selected == 1)
            {

                int bytes_sent = -strlen(file_name);
                size_t count = 0;
                char msg_buf[BUFSZ];
                memset(msg_buf, 0, BUFSZ);

                // size of header + content + strlen(/end)
                FILE *file = fopen(file_name, "r");
                long total_bites_to_send = get_file_size(file) + strlen(file_name) + 4;

                while (1)
                {
                    memset(msg_buf, 0, BUFSZ);
                    int bytes_to_send = total_bites_to_send - bytes_sent;
                    if (bytes_to_send > 500)
                    {
                        bytes_to_send = 500;
                    }
                    buf_file_range(file, bytes_sent, bytes_sent + bytes_to_send, msg_buf, total_bites_to_send, file_name);

                    count = send(s, msg_buf, bytes_to_send + 1, 0);

                    bytes_sent += bytes_to_send;
                    if (count != bytes_to_send + 1)
                    {
                        logexit("send");
                    }
                    // last message was sent
                    if (bytes_to_send < 500)
                        break;
                }
                fclose(file);

                memset(buf, 0, BUFSZ);
                unsigned total = 0;
                while (1)
                {
                    count = recv(s, buf + total, BUFSZ - total, 0);
                    if (count == 0)
                    {
                        // Connection terminated.
                        break;
                    }
                    total += count;
                }
                printf("%s\n", buf);
                close(s);
            }
            else
            {
                printf("no file selected! \n");
            }
        }
    }

    exit(EXIT_SUCCESS);
}