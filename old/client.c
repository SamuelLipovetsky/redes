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

char *concatenate_strings(const char *str1, const char *str2)
{
	size_t length1 = strlen(str1);
	size_t length2 = strlen(str2);
	size_t length3 = length1 + length2 + strlen("\\END") + 1;

	char *result = (char *)malloc(length3 * sizeof(char));

	strcpy(result, str1);
	strcat(result, str2);
	strcat(result, "/end");
	
	return result;
}
/*
Parses what command was given
char* str = full comand read by scanf
char** remainingString = everything in the str parameter that isnt a explicit command
Returns 1 if command is select file, 0 if it is send file and -1 if it isnt neither.
*/
int divide_string(const char *str, char **remainingString)
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
/*
Checks if a file ends with allowed extensions
char * str = name of the file ,it should be in "/.../.../.../helloWorld.py" format
returns 1 if extension is valid ,0 otherwise
*/
int ends_with_valid_extension(const char *str)
{
	const char *validExtensions[] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};
	int numValidExtensions = 6;

	// int length = strlen(str);

	const char *extension = strrchr(str, '.');

	for (int i = 0; i < numValidExtensions; i++)
	{
		if (strcmp(extension, validExtensions[i]) == 0)
		{
			return 1; // True
		}
	}

	return 0; // False
}
#include <stdio.h>
#include <stdlib.h>

/*
read a file and store it as a string
char *filename = path to location of file
returns pointer to allocated array of char with the whole file in it
*/
char *read_file_string(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	if (file == NULL)
	{
		fprintf(stderr, "Failed to open file: %s\n", filename);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long fileLength = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *buffer = (char *)malloc(fileLength + 1);
	if (buffer == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for file contents.\n");
		fclose(file);
		return NULL;
	}

	size_t bytesRead = fread(buffer, 1, fileLength, file);
	if (bytesRead != fileLength)
	{
		fprintf(stderr, "Error reading file: %s\n", filename);
		fclose(file);
		free(buffer);
		return NULL;
	}
	buffer[fileLength] = '\0';

	fclose(file);
	return buffer;
}

int min(int a, int b)
{
	if (a < b)
		return a;
	return b;
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

		int command_type = divide_string(buf, &file_name);
		// select file
		if (command_type == 1)
		{
			if (!ends_with_valid_extension(file_name))
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

				char *file_content = read_file_string(file_name);
				char *whole_msg_to_send = concatenate_strings(file_name, file_content);
				char pct_to_send[500];
				size_t remaining_bytes = strlen(whole_msg_to_send);
				size_t bytes_sent = 0;
				size_t count = 0;
				size_t received = 0;
				unsigned total = 0;
				while (bytes_sent < strlen(whole_msg_to_send))
				{
				
					total = 0;
					int bytes_to_send = min(500, remaining_bytes);
					memset(pct_to_send, 0, 500);
					
					strncpy(pct_to_send, whole_msg_to_send + bytes_sent, bytes_to_send+1);
					
					bytes_sent += bytes_to_send;
					count += send(s, pct_to_send, strlen(pct_to_send) + 1, 0);
					printf("\n%s\n",pct_to_send);
				
				}
				// count += send(s, pct_to_send, strlen(pct_to_send) + 1, 0);
	
				memset(buf, 0, BUFSZ);
				total = 0;
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
				// printing answer from server
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
