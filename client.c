#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

// Functions Declaration 
void port(int, char **);
void retr(int, char **);
void storappe(int, char **);

char namedPipe[50];
int i;
int main(int argc, char *argv[])
{
    char message[255], buff[255];
    int server, portNumber, pid, n;
    char *command;
    char **args = malloc(10 * sizeof(char *));
    struct sockaddr_in servAdd; // server socket address
    char *responseCode;

    if (argc != 3) //check for correct arguments while executing client
    {
        printf("Call model: %s <IP Address> <Port Number>\n", argv[0]);
        exit(0);
    }

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) // checking for socket creation
    {
        fprintf(stderr, "Cannot create socket\n");
        exit(1);
    }

    servAdd.sin_family = AF_INET;
    sscanf(argv[2], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t)portNumber);

    if (inet_pton(AF_INET, argv[1], &servAdd.sin_addr) < 0)
    {
        fprintf(stderr, " inet_pton() has failed\n");
        exit(2);
    }

    if (connect(server, (struct sockaddr *)&servAdd, sizeof(servAdd)) < 0)
    {
        fprintf(stderr, "connect() has failed, exiting\n");
        exit(3);
    }

    read(server, message, 255);
    fprintf(stderr, "%s\n", message);

    pid = fork();

    if (pid)
        while (1) /* reading server's messages */
            if ((n = read(server, message, 255)))
            {
                message[n] = '\0';
                fprintf(stderr, "%s", message);
                responseCode = strtok(message, " ");
                if (!strcasecmp(responseCode, "221"))
                {
                    close(server);
                    kill(pid, SIGTERM);
                    exit(0);
                }
            }

    if (!pid) /* sending messages to server */
        while (1)
        {
            if ((n = read(0, message, 255)))
            {
                message[n] = '\0';
                write(server, message, strlen(message) + 1);
                // strcpy(buff, message);
                command = strtok(message, " \n\0");
                i = 0;
                do
                {
                    args[i++] = strtok(NULL, " \n\0");
                } while (args[i - 1] != NULL);
                i--;
                if (!strcasecmp(command, "PORT"))
                {
                    port(i, args);
                }
                if (!strcasecmp(command, "RETR"))
                {
                    retr(i, args);
                }
                if (!strcasecmp(command, "STOR") || !strcasecmp(command, "APPE"))
                {
                    storappe(i, args);
                }
                if (!strcasecmp(command, "QUIT"))
                {
                    close(server);
                    exit(0);
                }
            }
        }
}

void port(int i, char **args) 
{                   // PORT request from client must send 1 argument with it
    if (i != 1)
    {
        printf("Syntax error in parameters or arguments.");
        return;
    }
    strcpy(namedPipe, "/tmp/");
    strcat(namedPipe, args[0]);
}

void retr(int i, char **args) 
{                   // RETR will send info and receive copy of file
    int fd1, fd2;
    char buffer[100];
    long int n1, counter;
    char *transferStaus;
    if (i != 1)
    {
        printf("Client Error: Syntax error in parameters or arguments.\n", i);
        return;
    }
    if ((fd1 = open(namedPipe, O_RDONLY)) == -1) //NamedPipe is assigned to FD1
    {
        perror("Client Error: Can't open Named Pipe: ");
        return;
    }
    if ((fd2 = open(args[0], O_CREAT | O_WRONLY | O_TRUNC, 0700)) == -1)
    {                                               // file is assigned to FD2
        perror("Client Error: Can't Create File: ");
        return;
    }
    if (!fork())        //child process will perform the download
    {
        printf("Client: Receiveing File ...\n");
        while ((n1 = read(fd1, buffer, 100)) > 0) //reading from pipe
        {
            if (write(fd2, buffer, n1) != n1) // writing to file on client loc
            {
                perror("Client Error: Error writing to file: ");
                close(fd1);
                close(fd2);
                exit(0);
            }
        }
        if (n1 == -1)
        {
            perror("Client Error: Error Reading from Pipe");
            close(fd1);
            close(fd2);
            exit(0);
        }
        close(fd1);
        close(fd2);
        printf("Client: File Saved.\n");
        exit(0);
    }
    close(fd1);
    close(fd2);
}

void storappe(int i, char **args)
{                   // STOR and APPE will perform same function as it
    int fd1, fd2;   // will just send the file from client and rest server will do
    char buffer[100];
    long int n1, counter;
    if (i != 1)
    {
        printf("Client Error: Syntax error in parameters or arguments.\n");
        return;
    }
    if ((fd1 = open(namedPipe, O_WRONLY)) == -1) //namedPipe is assigned to FD1
    {
        printf("Client Error: Can't open data connection.\n");
        return;
    }
    if ((fd2 = open(args[0], O_RDONLY)) == -1) // FD2 is assigned to file
    {
        perror("Client Error: File unavailable.");
        return;
    }
    printf("Client: transfer starting.");
    if (!fork())
    {  // reading from file and writing it to namedPipe from where server will read
        while ((n1 = read(fd2, buffer, 100)) > 0) 
        {

            if (write(fd1, buffer, n1) != n1)
            {
                perror("Client Erro: Can not write");
                exit(0);
            }
        }
        if (n1 == -1)
        {
            perror("Client Error: Can't read");
            exit(0);
        }
        close(fd1);
        close(fd2);
        printf("Client: File Transfer completed.\n");
        exit(0);
    }
    close(fd1);
    close(fd2);
}

