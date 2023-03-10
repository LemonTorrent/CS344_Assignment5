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

// Function checks to make sure key is long enough to decrypt text
bool testKeyLen(char* ptext, char* key){

    // Initialize variables

    int lenptext, lenkey;
    struct stat buf1;
    struct stat buf2;

    stat(ptext, &buf1);
    stat(key, &buf2);

    // Get the size of each file from the statistics

    lenptext = buf1.st_size;
    lenkey = buf2.st_size;

    // If the key is shorter then the length of the text, return error
    
    if (lenkey < lenptext){
        //perror("Error");
        //perror("Key too short for plaintext");
        fprintf(stderr, "Error: Key too short for plaintext\n", "string format", strlen("Key too short for plaintext\n"));
        exit(1);
        //return false;
    }

    return true;
}

// Retrieves contents of file
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
    //
    // Set newline and end of text characters to NULL
    readBuffer[strlen(readBuffer)] = '\0'; 
    readBuffer[strcspn(readBuffer, "\n")] = '\0'; 

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
    //

    bool validKey = testFileName(argv[2]);
    
    // Initialize variables to decrypt
    char *plaintext;
    char *key;
    char *ciphertext;
    char *decryptedtext;
    

    // Note: in this case plaintext refers to the ciphertext
    fflush(stdout);
    plaintext = fileContents(argv[1]);
    //

    // Get key from file
    key = fileContents(argv[2]);
    //printf("Returned key '%s'\n", key);


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


    char separate = '@';
    char terminateMssg = '#';
    char decryptSignal = '!';
    

    char temp [75000];
    memset(temp,0,sizeof(temp));

    // send information that we are decrypting
    charsWritten = send(socketFD, &decryptSignal, 1, 0);    // 23 corresponds to end of transmit block
    charsWritten = send(socketFD, &separate, 1, 0);    // 23 corresponds to end of transmit block
    
    // Send message to server
    
    // Send plaintext to server
    charsWritten = send(socketFD, plaintext, strlen(plaintext), 0);

    // If error, print that there was an error writing
    
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

    
    memset(buffer, '\0', sizeof(buffer));

    
    //memset(buffer, '\0', 150000);
    //char* current = buffer; // pointer to beginning of buffer

    char result [150000];
    char* current = result;

    // Receive text from server
    while (strstr(result, "#") == NULL) {
        
        charsRead = recv(socketFD, current, 150000, 0);
        
        current = current + charsRead; // move pointer to end of string
        
    }

    // If the server returns ##, it was a sign that we were using the wrong server
    if (strcmp(result, "##") == 0) {
        fprintf(stderr, "Error: Using encryption server for decryption\n", "string format", strlen("Error: Using encryption server for decryption\n"));
        printf("Error: Using encryption server for decryption\n");
        // Close the socket
        close(socketFD); 
        return 0;
    }
    
    // Otherwise the current text is the decrypted plaintext
    result[strcspn(result, "#")] = '\0'; 
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