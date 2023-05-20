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

char *get_full_filename(const char *str)
{
    // printf("\n----%s----\n",str);
    const char *patterns[] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};
    int numPatterns = 6;

    char *result = NULL;

    const char *dotPosition = strrchr(str, '.');
    int name_size = dotPosition - str;

    if (dotPosition != NULL)
    {
        // Get the substring starting from the dot position
        const char *substring = dotPosition;

        // Find the first matching pattern
        for (int i = 0; i < numPatterns; i++)
        {
            int patternLength = strlen(patterns[i]);

            if (strncmp(substring, patterns[i], patternLength) == 0)
            {

                int total_size = name_size + patternLength + 1;
                result = (char *)calloc(5, (total_size + 1) * sizeof(char));
                strncpy(result, str, total_size - 1);

                break;
            }
        }
    }
   
    return result;
}
void create_update_file(const char *fileName, const char *fileContent, char mode[1])
{
    FILE *file = fopen(fileName, mode);
    if (file == NULL)
    {
        fprintf(stderr, "Failed to create file: %s\n", fileName);
        return;
    }

    fprintf(file, "%s", fileContent);

    fclose(file);
    // printf("File created successfully: %s\n", fileName);
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
        //reads the first message that should contain the name of the file
        size_t count = recv(csock, buf, 501, 0);
        char *full_file_name = get_full_filename(buf);
       
        // removing complete path of filename
        char *file_name = strrchr(full_file_name, '/');
        file_name++;
        //flag for already existing files
        int already_exists = 0;
        if (access(file_name, F_OK) != -1)
        {
            already_exists = 1;
        }
        char file_content[BUFSIZ];
        memset(file_content, 0, BUFSIZ);

        // removing the name from message
        strcpy(file_content, buf + strlen(full_file_name));
        // printf("\n%d\n",count);
        if (count>500){
        create_update_file(file_name, file_content, "w");
        
        while (1)
        {
            memset(buf, 0, BUFSIZ);
            size_t count = recv(csock, buf, 501, 0);
            // printf("\n--%s--\n",buf);
            //searching for /end
            char *end = strrchr(buf, '/');
            if (end != NULL)
            {
                char *slashPtr = strchr(buf, '/');
                if (slashPtr != NULL)
                {
                    *slashPtr = '\0';
                }
                 create_update_file(file_name, buf, "a");
                 break;

            }
            //no end found
            create_update_file(file_name, buf, "a");

        }
        }
        else{
             char *end = strrchr(file_content, '/');
            if (end != NULL)
            {
                char *slashPtr = strchr(file_content, '/');
                if (slashPtr != NULL)
                {
                    *slashPtr = '\0';
                }
            }
             create_update_file(file_name, file_content, "w");

        }


        char answer[100];
        memset(buf, 0, BUFSIZ);
        strcpy(answer, full_file_name);

        if (already_exists)
        {
            strcat(answer, " file overwritten");
        }
        else
        {
            strcat(answer, " received");
        }

        count = send(csock, answer, strlen(answer) + 1, 0);
        if (count != strlen(answer) + 1)
        {
            logexit("send");
        }

        free(full_file_name);

        close(csock);
    }

    exit(EXIT_SUCCESS);
}