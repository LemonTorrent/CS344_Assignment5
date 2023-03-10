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
 * Function that takes key and plaintext as input, then outputs encrypted string.
*/
char * encrypt(char* key, char *plaintext) {
    
    // Initialize variables
    int ptextChar;
    int keyChar;
    int tempInt;
    char encryptChar;
    int lenText = strlen(plaintext);

    // Clear contents of string that will store ciphertext
    char* ctext = malloc(75000 * sizeof(char));

    // Iterate through the entire plaintext to encrypt each char
    for (int i = 0; i < strlen(plaintext); i++){
        // Change chars to integers
        if (plaintext[i]==' ' || (plaintext[i] >= 'A' && plaintext[i] <= 'Z')){

            // If char is space, change to 26. Otherwise get int by ascii value
            if (plaintext[i] == ' ') {
                ptextChar = 26;
            } else {
                ptextChar = plaintext[i] - 65;
            }

            // If key char is space, change to 26. Otherwise get int by ascii value
            if (key[i] == ' ') {
                keyChar = 26;
            } else {
                keyChar = key[i] - 65;
            }

            // Encrypt by adding chars, then mod 27

            tempInt = (ptextChar + keyChar) % 27;       // encrypt char
            if (tempInt == 26) {
                encryptChar = ' ';
            } else {
                encryptChar = tempInt + 65;
            }

            // Set ciphertext char to encrypted char
            ctext[i] = encryptChar;
        }
    }

    // set last char to null
    ctext[strlen(ctext)] = '\0';

    // printf("encryped text = %s, length %i\n", ctext, strlen(ctext));

    return ctext; 
}


int main(int argc, char *argv[]){
  // Intialize variables
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

    //printf("SERVER: Connected to client running at host %d port %d\n", 
    //                      ntohs(clientAddress.sin_addr.s_addr),
    //                      ntohs(clientAddress.sin_port));

    // Get the message from the client and display it
    memset(buffer, '\0', 150000);
    char* current = buffer; // pointer to beginning of buffer

    // Wait until server receives end of message char
    while (strstr(buffer, "#") == NULL) {
      //printf("SERVER: Buffer: '%s'\n", buffer);
      //printf("Size of buffer = %i\n", strlen(buffer));
      // printf("Inside while loop\n");
      charsRead = recv(connectionSocket, current, 150000, 0);
      current = current + charsRead; // move pointer to end of string
    }

    //printf("SERVER: Size of buffer = %i\n", strlen(buffer));

    //printf("Total message : %s\n", buffer);

    /*
    // Read the client's message from the socket
    charsRead = recv(connectionSocket, buffer, 255, 0); 
    if (charsRead < 0){
      error("ERROR reading from socket");
    }

    char* input;
    strcpy(input, buffer);

    */

    //printf("SERVER: I received this from the client: \"%s\"\n", buffer);

    // Initialize variables that will be used to send encrypt
    char plaintext[7500];
    // clear contents of variable
    memset(plaintext, '\0', 7500);

    char key [7500];
    // clear contents of variable
    memset(key, '\0', 7500);

    char decryptSignal = '!';
    char * decryptTermination = "!#";
    char terminateMssg = '#';
    
    // Tokenize original string to get key and plaintext
    char* token = strtok(buffer, "@");

    // If first token is !, the message came from the decryption client
    if (strstr(token, "!")){
      printf("SERVER: Wrong server. Needs decryption\n");
      charsWritten = send(connectionSocket, &terminateMssg, 1, 0);
      charsWritten = send(connectionSocket, &terminateMssg, 1, 0);
      
      //charsWritten = send(connectionSocket, decryptTermination, strlen(decryptTermination), 0);

    } else {
      // copy to plaintext variable
      strcpy(plaintext, token);
      
      // save length of plaintext in variable
      int lenPlaintext = strlen(plaintext);

      // Get key from token
      token = strtok(NULL, "\n");
      strcpy(key, token);
      
      // Replace end of message char with null char
      key[strcspn(key, "#")] = '\0'; 
      
      // Encrypt plaintext
      encryptTxt = encrypt(key, plaintext);

      //printf("SERVER: ciphertext '%s'\n", encryptTxt);
      //printf("Len plaintext %i\n", lenPlaintext);

      //int flag = 1; 
      //setsockopt(connectionSocket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

      // Send encrypted text to client
      charsWritten = send(connectionSocket, encryptTxt, lenPlaintext + 1, 0);

      // Send terminating message char to client
      charsWritten = send(connectionSocket, &terminateMssg, 1, 0);
      
      if (charsWritten < 0){
        error("SERVER: ERROR writing to socket");
      }
      if (charsWritten < 0){
          //printf("CLIENT: Num written %i\n", charsWritten);
          error("CLIENT: ERROR writing to socket");
      }
    }

    // Close the connection socket for this client
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}
