#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];

    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(9999);

    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
    printf("Connected\n");

    while(1)
    {
        printf("Enter message : ");
        scanf("%s" , message);

        //Send some data
        if( send(sock , message , strlen(message) , 0) < 0)
        {
            perror("Send failed");
            return 1;
        }

        //Receive a reply from the server
        memset(server_reply, 0, sizeof(server_reply));
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            perror("recv failed");
            break;
        }

        printf("Server reply :\n%s\n", server_reply);
    }

    close(sock);
    return 0;
}






















/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
