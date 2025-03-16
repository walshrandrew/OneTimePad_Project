#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

//FUNCTION: Receives from client while checking all data is received
int justGonnaTakeIt(int s, char *buf, size_t len)
{
  int received = 0;
  int remaining = len;
  int n;

  while(received < len)
  {
    n = recv(s, buf + received, remaining, 0);
    if(n == -1) { break; }
    received += n;
    remaining -= n;
  }
  len = received; //number received
  fprintf(stderr, "Bytes Received: %ld\n", len);
  return n == -1? -1:0; //-1 failure, 0 success
}

// FUNCTION: Sends to client while checking all data is sent
int justGonnaSendIt(int s, char *buf, size_t len)
{
  int sent = 0;
  int remaining = len;
  int n;
  fprintf(stderr, "Bytes started with: %ld\n", len);

  while(sent < len)
  {
    n = send(s, buf + sent, remaining, 0);
    if(n == -1) { break; }
    sent += n;
    remaining -= n;
  }
  len = sent; //number sent to server
  fprintf(stderr, "Bytes sent to client from server: %ld\n", len);
  fprintf(stderr, "Bytes remaining: %d\n", remaining);
  return n == -1? -1:0; //-1 failure, 0 success
}

//FUNCTION: Read file content into a buffer for packages.
char *readFiles(const char *file, long size)
{
  FILE *fp = fopen(file, "r");
  if(fp == NULL)
  {
    fprintf(stderr, "Error, can't read NULL files\n");
    exit(1);
  }

  char *buffer = malloc(size + 1);
  size_t bytes = fread(buffer, 1, size, fp);
  //buffer[strcspn(buffer, "\n")] = '\0';
  
  fclose(fp);
  return buffer;
}

//FUNCTION: decrypt
void otpdecryption(char *text, char* key, long size)
{
  int numText, numKey = 0;
  char valid[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  for (long i = 0; i < size; i++)
  {
    for(int j = 0; j < 27; j++)
    {
      if(text[i] == valid[j])
      {
        numText = j;
        break;
      }
    }
    
    for(int j = 0; j < 27; j++)
    {
      if(key[i] == valid[j])
      {
        numKey = j;
        break;
      }
    }

    int decrypted = (numKey - numText + 27) % 27;
    text[i] = valid[decrypted];
  }
}



int main(int argc, char *argv[]){
  int connectionSocket, charsRead;
  char buffer[256];
  char hostName[256];
  char text[80000];
  char key[80000];
  char res[80000];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

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
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1)
  {
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept");
    }

    printf("SERVER: Connected to client running at host %d port %d\n", ntohs(clientAddress.sin_addr.s_addr), ntohs(clientAddress.sin_port));

    // Get the message from the client and display it
    memset(buffer, '\0', 256);
    // Read the client's message from the socket
    charsRead = recv(connectionSocket, buffer, 255, 0);
    if (charsRead < 0){
      error("ERROR reading from socket");
    }
    printf("SERVER: I received this from the client: \"%s\"\n", buffer);

    // Validate incoming connections. Close if not same as server name
    if(strcmp(buffer, "dec") != 0 )
    {
      fprintf(stderr, "Wrong client tried to connect to me!\n");
      close(connectionSocket);
      continue;
    }
    else 
    {
      fprintf(stderr, "Successfully connected!\n");
    }

    
    // ---- HAVING A BABY!!! ----
    pid_t pid = fork();
    if(pid < 0) 
    {
      perror("error forking your mom!"); 
      exit(1);
    }
    if(pid == 0) 
    {
      close(listenSocket);
      // ---- BEGIN RECEIVING PACAKGES ----
      long fileSize, keySize;
      char keycpy[80000];
      char rescpy[80000];
      char ack[3];

      //recv file first
      recv(connectionSocket, &fileSize, sizeof(fileSize), 0);
      send(connectionSocket, ack, sizeof(ack), 0);
      justGonnaTakeIt(connectionSocket, rescpy, fileSize);   
      send(connectionSocket, ack, sizeof(ack), 0);


      //recv key
      recv(connectionSocket, &keySize, sizeof(keySize), 0);
      send(connectionSocket, ack, sizeof(ack), 0);
      justGonnaTakeIt(connectionSocket, keycpy, keySize);
      send(connectionSocket, ack, sizeof(ack), 0);


      //read them and encyrpt the 'res' file
      fprintf(stderr, "received filetext: %s\n", rescpy);

      //encrypt here:
      otpdecryption(rescpy, keycpy, fileSize);
      fprintf(stderr, "Encrypted filetext: %s\n", rescpy);

      //send back
      justGonnaSendIt(connectionSocket, rescpy, fileSize);
      close(connectionSocket);
      exit(0);
    }



    // Send a Success message back to the client
    //charsRead = send(connectionSocket, "I am the server, and I got your message", 39, 0); 
    //if (charsRead < 0){
      //error("ERROR writing to socket");
    //}
    // Close the connection socket for this client
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}
