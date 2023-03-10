#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

char machine [] = "localhost";
// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

// Tests to see if the file names describe files in the current directory
bool testFileName(char* fname) {
    //printf("Inside test file name\n");
    int file_descriptor;

    int fd = open(fname, O_RDONLY);
	if (file_descriptor == -1){
		printf("open() failed on \"%s\"\n", fname);
		error("Error opening file");
        return false;
		// exit(1);
	}

    // close(file_descriptor);

    return true;
}

// Function checks to make sure key is long enough to encrypt text
bool testKeyLen(char* ptext, char* key){

    // Initialize variables

    int lenptext, lenkey;
    struct stat buf1;
    struct stat buf2;

    stat(ptext, &buf1);
    // stat("plaintext6.txt", &buf1);
    stat(key, &buf2);

    // Get the size of each file from the statistics

    lenptext = buf1.st_size;
    lenkey = buf2.st_size;

    // If the key is shorter then the length of the text, return error
    
    if (lenkey < lenptext){
        //perror("Error");
        //perror("Key too short for plaintext");
        fprintf(stderr, "Error: Key too short for plaintext\n", "string format", strlen("Error: Key too short for plaintext\n"));
        printf("Error: Key too short for plaintext\n");
        exit(1);
        //return false;
    }

    return true;
}

// Retrieves contents of file, makes sure that characters are good input
char * fileContents(char* filename) {
    // Open file
    int fd = open(filename, O_RDONLY);
    if (fd == -1){
		printf("open() failed on \"%s\"\n", filename);
		perror("Error");
		exit(1);
	}

    // We allocate a buffer to read from the file
    char* readBuffer = malloc(75000 * sizeof(char));
    int howMany = read(fd, readBuffer, 75000);
    
    // Set newline and end of text characters to NULL
    readBuffer[strlen(readBuffer)] = '\0'; 
    readBuffer[strcspn(readBuffer, "\n")] = '\0'; 

    // If file contains char that aren't accepted, print error
    if (strstr(readBuffer, "$") != NULL || strstr(readBuffer, "*") != NULL || 
        strstr(readBuffer, "!") != NULL || strstr(readBuffer, "(") != NULL || 
        strstr(readBuffer, "#") != NULL || strstr(readBuffer, "-") != NULL) {
            fprintf(stderr, "Error: Bad characters in file\n", "string format", strlen("Error: Bad characters in file\n"));
            //printf("Error: Bad characters in file\n");
            exit(1);
        }

    return readBuffer;

}

int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    char buffer[256];
    // Check usage & args
    if (argc < 4) { 
        fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
        exit(0); 
    } 

    bool validPText = testFileName(argv[1]);
    //printf("Is valid ptext: %i\n", validPText);

    bool validKey = testFileName(argv[2]);
    //printf("Is valid ptext: %i\n", validKey);

    bool compLen = testKeyLen(argv[1], argv[2]);
    //printf("Long enough keylen: %i\n", compLen);
    
    //char plaintext [7500];
    char *plaintext;
    char *key;
    char *ciphertext;
    char *decryptedtext;
    

    fflush(stdout);
    plaintext = fileContents(argv[1]);
    //printf("Returned plaintext '%s'\n", plaintext);

    key = fileContents(argv[2]);

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (socketFD < 0){
        error("CLIENT: ERROR opening socket");
    }

    // Set up the server address struct
    setupAddressStruct(&serverAddress, atoi(argv[3]), machine);

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        error("CLIENT: ERROR connecting");
    }

    // Set characters that represent separation and termination
    char separate = '@';
    char terminateMssg = '#';

    char temp [75000];
    memset(temp,0,sizeof(temp));


    // Send plaintext to server
    charsWritten = send(socketFD, plaintext, strlen(plaintext), 0); 

    // If less char are written then expected, print error
    
    if (charsWritten < strlen(plaintext)){
        printf("CLIENT: Num written %i\n", charsWritten);
        printf("CLIENT: ERROR writing to socket");
    }

    // Send key to server
    charsWritten = send(socketFD, &separate, 1, 0);    // 23 corresponds to end of transmit block
    charsWritten = send(socketFD, key, strlen(key), 0); 
    charsWritten = send(socketFD, &terminateMssg, 1, 0);    // 23 corresponds to end of transmit block

    if (charsWritten < 0){
        printf("CLIENT: Num written %i\n", charsWritten);
        error("CLIENT: ERROR writing to socket");
    }

    // Clear out the buffer again for reuse
    memset(buffer, '\0', sizeof(buffer));
    
    // Create new variable to receive results

    char result [150000];
    char* current = result;


    // Get return message from server
    charsRead = recv(socketFD, result, 150000, 0);
    
    // Set last character to newline
    result[strlen(result)] = '\0';
    //printf("Length plaintext = %i\n", strlen(result));
    printf("%s\n", result);

    if (charsRead < 0){
        error("CLIENT: ERROR reading from socket");
    }
    //printf("CLIENT: I received this from the server: \"%s\"\n", result);

    // Close the socket
    close(socketFD); 
    return 0;
}