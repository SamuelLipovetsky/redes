#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 4000

void usage(int argc, char **argv)
{
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

/*
recovers file name from full message received
char * str = string of full message received
returns the full name of file to be created
*/
char *get_full_filename(const char *str)
{

    const char *patterns[] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};
    int num_patterns = 6;

    char *result = NULL;


    const char *dot_position = strrchr(str, '.');
    if (dot_position == NULL)
    {
        return NULL;
    }
    int name_size = dot_position - str;

    if (dot_position != NULL)
    {
        // Get the substring starting from the dot position
        const char *substring = dot_position;

        // Find the first matching pattern
        for (int i = 0; i < num_patterns; i++)
        {
            int pattern_length = strlen(patterns[i]);

            if (strncmp(substring, patterns[i], pattern_length) == 0)
            {

                int total_size = name_size + pattern_length + 1;
                result = (char *)calloc(5, (total_size + 1) * sizeof(char));
                strncpy(result, str, total_size - 1);

                break;
            }
        }
    }

    return result;
}
/*
simple fucntion to create a file
char *file_name = name of the file to be created
char *file_content = content of file to be created
*/
void create_file(const char *file_name, const char *file_content)
{
    FILE *file = fopen(file_name, "w");
    if (file == NULL)
    {

        return;
    }

    fprintf(file, "%s", file_content);

    fclose(file);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage)))
    {
        logexit("bind");
    }

    if (0 != listen(s, 10))
    {
        logexit("listen");
    }

    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);
    char buf[BUFSZ];
    while (1)
    {
        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1)
        {
            logexit("accept");
        }

        memset(buf, 0, BUFSIZ);
        char answer[100];
        char *full_file_name;
        // flags for exiting/overwritten messages
        int exit = 1;
        int already_exists = 0;

        size_t count = recv(csock, buf, BUFSZ - 1, 0);
        if (count == -1)
        {
            memset(answer, 0, 100);
            strcpy(answer, "error receiving file");
            exit = 0;
        }

        else
        {
            full_file_name = get_full_filename(buf);
            if (full_file_name != NULL)
            {
                //checking if file already exissts
                if (access(full_file_name, F_OK) != -1)
                {
                    already_exists = 1;
                }
                char file_content[BUFSIZ];
                memset(file_content, 0, BUFSIZ);

                //checking for exit command
                if (strcmp(buf, "exit\\end"))
                {
                    exit = 0;
                    // removing the name from message
                    strcpy(file_content, buf + strlen(full_file_name));

                    // removing the \end from message
                    char *slashPtr = strchr(file_content, '\\');
                    if (slashPtr != NULL)
                    {
                        *slashPtr = '\0';
                    }
                    //writing file
                    create_file(full_file_name, file_content);
                }
            }
            else
            {
                exit = 1;
            }
        }
        //defining answer to be sent
        if (exit)
        {
            memset(answer, 0, 100);
            strcpy(answer, "connection closed");
            printf("connection closed\n");

        }
        else if (already_exists)
        {
            memset(answer, 0, 100);
            strcat(answer, "file ");
            strcat(answer, full_file_name);
            strcat(answer, " file overwritten");
            printf("%s\n", answer);
        }
        else
        {
            memset(answer, 0, 100);
            strcat(answer, "file ");
            strcat(answer, full_file_name);
            strcat(answer, " received");
            printf("%s\n", answer);
        }
        //sending answer
        count = send(csock, answer, strlen(answer) + 1, 0);
        if (count != strlen(answer) + 1)
        {
            logexit("send");
        }

        free(full_file_name);

        close(csock);
        //ending conectionf if exit flag 
        if (exit)
        {
            close(s);
            break;
        }
    }

    exit(EXIT_SUCCESS);
}