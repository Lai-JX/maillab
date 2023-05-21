#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#define swap16(x) ((((x)&0xFF) << 8) | (((x) >> 8) & 0xFF))

#define MAX_SIZE 65535

char buf[MAX_SIZE+1];

void mysend(int s_fd, const char* str, const char* error_msg) 
{
    if(send(s_fd, str, strlen(str), 0) == -1 )
    {
        perror(error_msg);
        exit(EXIT_FAILURE);
    }
    // printf("%s", str);
    printf("\033[0m\033[1;32m%s\033[0m", str);
}

int myresv(int s_fd) 
{
    int r_size;
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s\n", buf);
    return r_size;
}

void recv_mail()
{
    const unsigned short port = 110; // POP3 server port
    // const char* host_name = "pop.163.com"; // TODO: Specify the mail server domain name
    // const char* user = "****************@163.com"; // TODO: Specify the user
    // const char* pass = "****************"; // TODO: Specify the password
    const char* host_name = "pop.qq.com"; // TODO: Specify the mail server domain name
    const char* user = "**********@qq.com"; // TODO: Specify the user
    const char* pass = "****************"; // TODO: Specify the password
    char dest_ip[16];
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    s_fd = socket(AF_INET, SOCK_STREAM, 0);
    // 构建sockaddr_in
    struct sockaddr_in sockaddr_in_s;
    sockaddr_in_s.sin_family = AF_INET;
    sockaddr_in_s.sin_port = swap16(port);
    bzero(sockaddr_in_s.sin_zero,8);
    sockaddr_in_s.sin_addr.s_addr = inet_addr(dest_ip);
    printf("create socket\n");
    // 连接
    if (connect(s_fd, (struct sockaddr *)&sockaddr_in_s, sizeof(sockaddr_in_s)) != 0)
    {
        perror("connect error!\n");
        exit(EXIT_FAILURE);
    }

    // Print welcome message
    myresv(s_fd);

    // TODO: Send user and password and print server response
    sprintf(buf, "user %s\r\n", user);
    mysend(s_fd, buf, "Send user fail!\n");
    myresv(s_fd);

    sprintf(buf, "pass %s\r\n", pass);
    mysend(s_fd, buf, "Send pass fail!\n");
    myresv(s_fd);

    // TODO: Send STAT command and print server response
    mysend(s_fd, "STAT\r\n", "Send STAT fail!\n");
    myresv(s_fd);

    // TODO: Send LIST command and print server response
    mysend(s_fd, "LIST\r\n", "Send STAT fail!\n");
    myresv(s_fd);

    // TODO: Retrieve the first mail and print its content
    mysend(s_fd, "RETR 1\r\n", "Send STAT fail!\n");
    r_size = myresv(s_fd);
    int tol_size = atoi(buf+4);
    tol_size -= r_size;
    while (tol_size > 0)
    {
        r_size = myresv(s_fd);
        tol_size -= r_size;
    }

    // TODO: Send QUIT command and print server response
    mysend(s_fd, "QUIT\r\n", "QUIT command failed!\n");
    myresv(s_fd);

    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}
