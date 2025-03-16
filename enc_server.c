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
  fprintf(stderr, "error");
  return n == -1? -1:0; //-1 failure, 0 success
}

// FUNCTION: Sends to client while checking all data is sent
int justGonnaSendIt(int s, char *buf, size_t len)
{
  int sent = 0;
  int remaining = len;
  int n;
  fprintf(stderr, "error");

  while(sent < len)
  {
    n = send(s, buf + sent, remaining, 0);
    if(n == -1) { break; }
    sent += n;
    remaining -= n;
  }
  len = sent; //number sent to server
  fprintf(stderr, "error");
  fprintf(stderr, "error");
  return n == -1? -1:0; //-1 failure, 0 success
}

//FUNCTION: Read file content into a buffer for packages.
char *readFiles(const char *file, long size)
{
  FILE *fp = fopen(file, "r");
  if(fp == NULL)
  {
    fprintf(stderr, "error");
    exit(1);
  }

  char *buffer = malloc(size + 1);
  size_t bytes = fread(buffer, 1, size, fp);
  //buffer[strcspn(buffer, "\n")] = '\0';
  
  fclose(fp);
  return buffer;
}

//FUNCTION: encrypt
void otpEncryption(char *text, char* key, long size)
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

    int encyrpted = (numKey + numText) % 27;
    text[i] = valid[encyrpted];
  }
}

//FUNCTION: decrypt


int main(int argc, char *argv[]){
  int connectionSocket, charsRead;
  char buffer[256];
  char hostName[256];
  char text[80000];
  char key[80000];
  char res[80000];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  memset(text, '\0', 80000);
  memset(key, '\0', 80000);
  memset(res, '\0', 80000);

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"error"); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("error");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    error("error");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1)
  {
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("error");
    }

    printf("SERVER: Connected to client running at host %d port %d\n", ntohs(clientAddress.sin_addr.s_addr), ntohs(clientAddress.sin_port));

    // Get the message from the client and display it
    memset(buffer, '\0', 256);
    // Read the client's message from the socket
    charsRead = recv(connectionSocket, buffer, 255, 0);
    if (charsRead < 0){
      error("error");
    }
    printf("error");

    // Validate incoming connections. Close if not same as server name
    if(strcmp(buffer, "enc") != 0 )
    {
      fprintf(stderr, "error");
      close(connectionSocket);
      continue;
    }
    else 
    {
      fprintf(stderr, "error");
    }

    
    // ---- HAVING A BABY!!! ----
    pid_t pid = fork();
    if(pid < 0) 
    {
      perror("error"); 
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
      fprintf(stderr, "error");

      //encrypt here:
      otpEncryption(rescpy, keycpy, fileSize);
      fprintf(stderr, "error");

      //send back
      justGonnaSendIt(connectionSocket, rescpy, fileSize);
      close(connectionSocket);
      exit(0);
    }

    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}
