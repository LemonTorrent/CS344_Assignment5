#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define CHUNK_SIZE 100

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

/*
 * Function that takes key and ciphertext as input, then outputs decrypted string.
*/
char * decrypt(char* key, char *ciphertext) {

    // Initialize variables
    int cipherChar;
    int keyChar;
    int tempInt;
    char decryptChar;

    char* ptext = malloc(75000 * sizeof(char));

    // Iterate through ciphertext to decrypt
    for (int i = 0; i < strlen(ciphertext); i++){
        // Get integer equivalents of characters
        if (ciphertext[i] == ' ') {
            cipherChar = 26;
        } else {
            cipherChar = ciphertext[i] - 65;
        }

        if (key[i] == ' ') {
            keyChar = 26;
        } else {
            keyChar = key[i] - 65;
        }

        // Decrypt by subtracting mod 27

        tempInt = (cipherChar - keyChar + 27) % 27;       // decrypt char
        if (tempInt == 26) {
            decryptChar = ' ';
        } else {
            decryptChar = tempInt + 65;
        }

        // Append decrypted character to plaintext
        ptext[i] = decryptChar;
    }

    // Set last char to null

    ptext[strlen(ptext) + 1] = '\0';

    // Return plaintext

    return ptext; 
}


int main(int argc, char *argv[]){
  int connectionSocket, charsRead, charsWritten;
  char buffer[150000];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);
  
  char* encryptTxt;

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1){
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept");
    }

    // Get the message from the client and display it
    memset(buffer, '\0', 150000);
    char* current = buffer; // pointer to beginning of buffer

    // Receive text from client
    while (strstr(buffer, "#") == NULL) {
      //printf("SERVER: Buffer: '%s'\n", buffer);
      //printf("Size of buffer = %i\n", strlen(buffer));
      // printf("Inside while loop\n");
      charsRead = recv(connectionSocket, current, 150000, 0);
      current = current + charsRead; // move pointer to end of string
    }


    // Initialize variables. Note: Plaintext refers to ciphertext, I was just to scared to change the name

    char plaintext[7500];
    // clear contents of variable
    memset(plaintext, '\0', 7500);

    char key [7500];
    // clear contents of variable
    memset(key, '\0', 7500);
    
    // use tokenization to retreive information from sent string
    char* token = strtok(buffer, "@");
    //token = strtok(NULL, "\n");

    // Copy contents of token to use first token
    strcpy(plaintext, token);

    // Copy ciphertext that is found in second token
    token = strtok(NULL, "@");
    strcpy(plaintext, token);
    
    
    // Record length of plaintext
    int lenPlaintext = strlen(plaintext);

    // Get key by tokenization
    token = strtok(NULL, "\n");
    strcpy(key, token);
    
    // Remove terminating char from key
    key[strcspn(key, "#")] = '\0';
    
    // decrypt ciphertext
    encryptTxt = decrypt(key, plaintext);

    char terminateMssg = '#';

    // Send plaintext to client
    charsWritten = send(connectionSocket, encryptTxt, lenPlaintext, 0); 

    // Send terminating char to client
    charsWritten = send(connectionSocket, &terminateMssg, 1, 0);
    //printf("SERVER: Sent terminating char to client\n");
    if (charsWritten < 0){
      error("SERVER: ERROR writing to socket");
    }
    if (charsWritten < 0){
        //printf("CLIENT: Num written %i\n", charsWritten);
        error("CLIENT: ERROR writing to socket");
    }
    
    // Close the connection socket for this client
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}
