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
/*
Concatenates header - body - tails of message to be send
char *str1 = header of message
char *str = body of message
Retuns pointer to full message to be send
*/
char *concatenate_strings(const char *str1, const char *str2)
{

    // removig path from file name
    str1 = strrchr(str1, '/');
    str1++;

    size_t length1 = strlen(str1);
    size_t length2 = strlen(str2);
    size_t length3 = length1 + length2 + strlen("\\end") + 1;

    char *result = (char *)malloc(length3 * sizeof(char));
    if (result == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    strcpy(result, str1);
    strcat(result, str2);
    strcat(result, "\\end");

    return result;
}
/*
Divide commands into command and file name
char *str = full command read from input
char **remainingString = file name read from input
returns 0 if command was "send file"
returns 1 if command was "select file"
returns 2 if command was "exit"
returns -1 if no command was found
*/
int divide_string(const char *str, char **remaining_string)
{
    const char *select_file_prefix = "select file";
    const char *send_file_prefix = "send file";
    const char *exit = "exit";

    if (strncmp(str, select_file_prefix, strlen(select_file_prefix)) == 0)
    {

        *remaining_string = strdup(str + strlen(select_file_prefix) + 1);

        size_t length = strlen(*remaining_string);
        if (length > 0 && (*remaining_string)[length - 1] == '\n')
        {
            (*remaining_string)[length - 1] = '\0';
        }

        return 1;
    }
    else if (strncmp(str, send_file_prefix, strlen(send_file_prefix)) == 0)
    {
        return 0;
    }
    else if (strncmp(str, exit, strlen(exit)) == 0)
    {
        return 2;
    }
    return -1;
}
/*
checks if a given file name ends with a valid extension
char * str = a file name
returns 1 if true , 0 if false
*/
int valid_extension(const char *str)
{
    const char *valid_extensions[] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};
    int num_valid_extensions = 6;

    const char *extension = strrchr(str, '.');
    if (extension != NULL)
    {
        for (int i = 0; i < num_valid_extensions; i++)
        {
            if (strcmp(extension, valid_extensions[i]) == 0)
            {
                return 1; // True
            }
        }
    }

    return 0; // False
}

/*
reads a whole file and copies it to a string
char * filename = name of the file to be read
returns alloacated string with content of the whole file
*/
char *file_to_string(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(file_length + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for file contents.\n");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_length, file);
    if (bytes_read != file_length)
    {
        fprintf(stderr, "Error reading file: %s\n", filename);
        fclose(file);
        free(buffer);
        return NULL;
    }

    fclose(file);
    return buffer;
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

        // getting comand type and file name
        int command_type = divide_string(buf, &file_name);

        // select file
        if (command_type == 1)
        {
            if (!valid_extension(file_name))
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
        // send file
        else if (command_type == 0)
        {
            // valid file selected
            if (file_selected == 1)
            {
                // creating socket
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

                // creating body and header of message to be sent
                char *file_content = file_to_string(file_name);
                char *msg_to_send = concatenate_strings(file_name, file_content);

                // sending message
                size_t count = send(s, msg_to_send, strlen(msg_to_send) + 1, 0);
                if (count != strlen(msg_to_send) + 1)
                {
                    logexit("send");
                }

                // waiting for response
                memset(buf, 0, BUFSZ);
                unsigned total = 0;
                while (1)
                {
                    count = recv(s, buf + total, BUFSZ - total, 0);
                    if (count == 0)
                    {
                        break;
                    }
                    total += count;
                }
                // printing response and closing connection
                printf("%s\n", buf);
                close(s);
                free(file_content);
            }
            else
            {
                printf("no file selected! \n");
            }
        }
        else if (command_type == 2)
        {
            // creating connection
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

            // creating exit message
            char msg_buf[BUFSZ];
            memset(msg_buf, 0, BUFSIZ);
            strcpy(msg_buf, "exit\\end");

            // sending exit message;
            size_t count = send(s, msg_buf, 10, 0);

            if (count != 10)
            {
                logexit("send");
            }
            // receiving answer from server.
            memset(buf, 0, BUFSZ);
            unsigned total = 0;
            while (1)
            {
                count = recv(s, buf + total, BUFSZ - total, 0);
                if (count == 0)
                {

                    break;
                }
                total += count;
            }
            // closing conection and printing anser
            printf("%s\n", buf);
            close(s);
            break;
        }
      
    }

    exit(EXIT_SUCCESS);
}