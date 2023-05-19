#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 4000
void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

char* concatenateStrings(const char* str1, const char* str2) {
    size_t length1 = strlen(str1);
    size_t length2 = strlen(str2);
    size_t length3 = length1 + length2 + strlen("\\END") + 1;

    char* result = (char*)malloc(length3 * sizeof(char));
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    strcpy(result, str1);
    strcat(result, str2);
    strcat(result, "/end");

    return result;
}

int divideString(const char* str, char** remainingString) {
    const char* selectFilePrefix = "select file";
    const char* sendFilePrefix = "send file";

  
    if (strncmp(str, selectFilePrefix, strlen(selectFilePrefix)) == 0) {
     
        *remainingString = strdup(str + strlen(selectFilePrefix)+1);
		
		size_t length = strlen(*remainingString);
        if (length > 0 && (*remainingString)[length - 1] == '\n') {
            (*remainingString)[length - 1] = '\0';
        }
        return 1;
    }
 
    else if (strncmp(str, sendFilePrefix, strlen(sendFilePrefix)) == 0) {
      
        return 0;
    }

    return -1;
}
int endsWithValidExtension(const char* str) {
    const char* validExtensions[] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};
    int numValidExtensions = 6;

    int length = strlen(str);

    if (length >= 4) {
        // const char* lastFourChars = str + length - 4;
		const char* extension = strrchr(str, '.');
		

        for (int i = 0; i < numValidExtensions; i++) {
            if (strcmp(extension, validExtensions[i]) == 0) {
                return 1; // True
            }
        }
    }

    return 0; // False
}
#include <stdio.h>
#include <stdlib.h>

char* readFileToString(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileLength = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(fileLength + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for file contents.\n");
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, fileLength, file);
    if (bytesRead != fileLength) {
        fprintf(stderr, "Error reading file: %s\n", filename);
        fclose(file);
        free(buffer);
        return NULL;
    }
    buffer[fileLength] = '\0';

    fclose(file);
    return buffer;
}


int main(int argc, char **argv) {
	if (argc < 3) {
		usage(argc, argv);
	}

	// printf("connected to %s\n", addrstr);
	int file_selected =0;
	char* file_name;
	while (1)
	{
	char buf[BUFSZ];
	memset(buf, 0, BUFSZ);
	// printf("Comando ");
	fgets(buf, BUFSZ-1, stdin);
	
	
	int command_type=divideString(buf,&file_name);
	// select file

	if (command_type==1){
	 	if (!endsWithValidExtension(file_name)){
			file_selected=0;
			printf("%s not valid! \n", file_name);
		}
		else if (access(file_name, F_OK) == -1){
			file_selected=0;
			printf("%s does not exist \n",file_name);
		}
		else {
			file_selected=1;
			printf("%s selected \n",file_name);
		}
	}
	else if( command_type==0){
		
		struct sockaddr_storage storage;
		if (0 != addrparse(argv[1], argv[2], &storage)) {
			usage(argc, argv);
		}

		int s;
		s = socket(storage.ss_family, SOCK_STREAM, 0);
		if (s == -1) {
			logexit("socket");
		}
		struct sockaddr *addr = (struct sockaddr *)(&storage);
		if (0 != connect(s, addr, sizeof(storage))) {
			logexit("connect");
		}

		char addrstr[BUFSZ];
		addrtostr(addr, addrstr, BUFSZ);

		if (file_selected==1){

		char * file_content = readFileToString(file_name);
		char * msg_to_send = concatenateStrings(file_name,file_content);
		// printf("----- %s---- \n",msg_to_send);
		size_t count = send(s, msg_to_send, strlen(msg_to_send)+1, 0);
		if (count != strlen(msg_to_send)+1) {
			logexit("send");
		}

		memset(buf, 0, BUFSZ);
		unsigned total = 0;
		while(1) {
			count = recv(s, buf + total, BUFSZ - total, 0);
			if (count == 0) {
				// Connection terminated.
				break;
			}
			total += count;
		}
		printf("%s\n",buf);
		close(s);
		}
		else{
			printf("no file selected! \n");
		}
	}
	}

	exit(EXIT_SUCCESS);
}
