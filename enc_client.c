#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <sys/stat.h>   // fstat


/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// FUNCTION: Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// FUNCTION: Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname)
{ 
  memset((char*) address, '\0', sizeof(*address));   // Clear out the address struct
  address->sin_family = AF_INET;  // The address should be network capable
  address->sin_port = htons(portNumber);  // Store the port number
  struct hostent* hostInfo = gethostbyname(hostname);   // Get the DNS entry for this host name
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

//FUNCTION: returns file size for measuring file content. > checks plaintext > keygen
long fsize(const char* filename) {
  struct stat st;
  stat(filename, &st);
  return st.st_size;
}

// FUNCTION: Sends to server while checking all data is sent
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
  fprintf(stderr, "Bytes sent to server: %ld\n", len);
  fprintf(stderr, "Bytes remaining: %d\n", remaining);
  return n == -1? -1:0; //-1 failure, 0 success
}

//FUNCTION: Receives from server while checking all data is received
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

//FUNCTION: Read file content into a buffer for sending packages.
char *readFiles(const char *file, long size)
{
  char validChar[28] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  char *buffer = malloc(size + 1);
  FILE *fp = fopen(file, "r");

  //Check if file is empty
  if(fp == NULL)
  {
    fprintf(stderr, "Error, can't read NULL files\n");
    exit(1);
  }

  //Check file for bad characters (anything not valid):
  for (size_t i = 0; i < size; i++)
  {
    if(strchr(validChar, buffer[i]) == NULL)
    {
      fclose(fp);
      free(buffer);
      exit(1);
    }
  }

  // Read file content
  size_t bytes = fread(buffer, 1, size, fp);
  buffer[bytes] = '\0';  // Null terminate

  // Debug: Print before newline removal
  fprintf(stderr, "[DEBUG] Before newline removal: '%s' (length: %zu)\n", buffer, bytes);

  // Ensure we remove only the final newline (if it's the last character)
  if (bytes > 0 && buffer[bytes - 1] == '\n') {
    buffer[bytes - 1] = '\0';
    bytes--;  // Adjust the byte count
  }


  // Debug: Print after newline removal
  fprintf(stderr, "[DEBUG] After newline removal: '%s' (length: %zu)\n", buffer, bytes);
  fprintf(stderr, "[DEBUG-SIZE] After newline removal: '%s' (length: %zu)\n", buffer, size);


  fclose(fp);
  return buffer;
}

/*
* argv[0] == hostname
* argv[1] == plaintext1 (file to be encoded)
* argv[2] == myshortkey (OTP)
* argv[3] == Port Number to connect to
*/
int main(int argc, char *argv[]) {
  int socketFD, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  char buffer[256];
  const char *key = argv[2];
  const char *file = argv[1];
  char msg[4] = "enc";
  char encryptedFile[100000];

  // Check usage & args
  if (argc < 3) { 
    fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
    exit(0); 
  } 
  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }
   // Set up the server address struct & connect
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");
  int val = 1;
  setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)); //allow reuse of port
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }
  // Get input message from user
  //printf("CLIENT: Enter text to send to the server, and then hit enter: "); //this was being stored in ciphertext1
  // Clear out the buffer array
  memset(buffer, '\0', sizeof(buffer));
  // Get input from the user, trunc to buffer - 1 chars, leaving \0
  //fgets(buffer, sizeof(buffer) - 1, stdin);
  // Remove the trailing \n that fgets adds
  //buffer[strcspn(buffer, "\n")] = '\0'; 

  // check key length to input file length (both have '\0')
  long filesize = fsize(file);
  long keysize = fsize(key);
  if(filesize > keysize)
  {
    fprintf(stderr, "Error: key '%s' is too short", key);
    exit(1);
  }
  // check keysize to input filesize, if keysize bigger, chop it to same size
  if(keysize > filesize)
  {
    keysize = filesize;
  }


  //Send "enc" to server for verification.
  if(justGonnaSendIt(socketFD, msg, strlen(msg)) == -1)
  {
    perror("justgonnasendit");
    fprintf(stderr, "We only sent %ld bytes because of error!\n", strlen(msg));
    exit(1);
  }


  // Read plaintext and keygen files to a buffer to send to server
  char *fileBuffer = readFiles(file, filesize);   //used malloc, remember to free
  char *keyBuffer = readFiles(key, keysize);      //used malloc, remember to free
  filesize = strlen(fileBuffer);
  keysize = strlen(keyBuffer);
  char ack[3];                                    //ack buffer

  // send size of file, then file
  
  send(socketFD, &filesize, sizeof(filesize), 0);
  recv(socketFD, ack, sizeof(ack), 0);
  justGonnaSendIt(socketFD, fileBuffer, filesize);
  free(fileBuffer);
  recv(socketFD, ack, sizeof(ack), 0);

  // send size of key, then file
  send(socketFD, &keysize, sizeof(keysize), 0);
  recv(socketFD, ack, sizeof(ack), 0);
  justGonnaSendIt(socketFD, keyBuffer, keysize);
  free(keyBuffer);
  recv(socketFD, ack, sizeof(ack), 0);


  //receive encrypted file from server
  if(justGonnaTakeIt(socketFD, encryptedFile, filesize) != 0)
  {
    perror("justgonnatakeit");
    fprintf(stderr, "Encyrpted file is too big\n");
    exit(1);
  }

  //send encrypted file and key to dec_client
  fprintf(stderr, "Encrypted filetext: %s\n", encryptedFile);
  //add \n to end of encryptedFile
  long encyrptedLength = strlen(encryptedFile);

  encryptedFile[encyrptedLength] = '\n';
  encryptedFile[encyrptedLength + 1] = '\0';
  fprintf(stdout, "%s", encryptedFile);

  close(socketFD); 
  return 0;
}
