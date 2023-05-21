#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"
#define swap16(x) ((((x)&0xFF) << 8) | (((x) >> 8) & 0xFF))

#define MAX_SIZE 4095

char buf[MAX_SIZE+1];

void mysend(int s_fd, const char* str, const char* error_msg) 
{
    if(send(s_fd, str, strlen(str), 0) == -1 )
    {
        perror(error_msg);
        exit(EXIT_FAILURE);
    }
    printf("\033[0m\033[1;32m%s\033[0m", str);
    // printf("%s", str);
}

void myresv(int s_fd) 
{
    int r_size;
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s\n", buf);
}

char* myfile2str(const char* msg)
{
    FILE *fp;
    if ((fp = fopen(msg, "r")) == NULL)
    {
        return (char *)msg;     // 非文件类型，说明为文本，直接返回
    }
    // 将文件指针移到结尾
    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    // 根据文件大小分配空间
    char *str = (char *)malloc(filesize);
    str[0] = 0;
    // 将文件指针移回开头
    fseek(fp, 0, SEEK_SET);
    fread(str, 1, filesize, fp);
    fclose(fp);
    return str;
}

// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const unsigned short port = 25; // SMTP server port
    // const char *EHLO = "EHLO 163.com\r\n";  // 不能换行两次（qq可以，163不能）
    // const char* host_name = "smtp.163.com"; // TODO: Specify the mail server domain name
    // const char* user = "****************@163.com"; // TODO: Specify the user
    // const char* pass = "****************"; // TODO: Specify the password
    // const char* from = "****************@163.com"; // TODO: Specify the mail address of the sender
    const char *EHLO = "EHLO qq.com\r\n";
    const char *host_name = "smtp.qq.com";         // TODO: Specify the mail server domain name
    const char* user = "**********@qq.com"; // TODO: Specify the user
    const char* pass = "****************"; // TODO: Specify the password
    const char* from = "**********@qq.com"; // TODO: Specify the mail address of the sender
    char dest_ip[16] = "127.0.0.1"; // Mail server IP address. // test时将dest_ip设置为127.0.0.1
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
    // printf("%s\n", dest_ip);


    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
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
    // printf("connected\n");
    // Print welcome message
    myresv(s_fd);

    // Send EHLO command and print server response
    mysend(s_fd, EHLO, "EHLO command failed!\n"); // 记得换行

    // TODO: Print server response to EHLO command
    myresv(s_fd);

    // TODO: Authentication. Server response should be printed out.
    mysend(s_fd, "AUTH login\r\n", "Authentication failed!\n");
    myresv(s_fd);

    char *base64_user = encode_str(user);
    mysend(s_fd, base64_user, "Authentication(user) failed!\n");
    mysend(s_fd, "\r\n", "Authentication(user) failed!\n");     // qq需要，163不能要
    free(base64_user);
    myresv(s_fd);

    char *base64_pass = encode_str(pass);
    mysend(s_fd, base64_pass, "Authentication(pass) enter failed!\n");
    mysend(s_fd, "\r\n", "Authentication(pass) failed!\n");     // qq需要，163不能要
    free(base64_pass);
    myresv(s_fd);

    // TODO: Send MAIL FROM command and print server response
    strcpy(buf, "MAIL FROM:<");
    strcat(buf, from);
    strcat(buf, ">\r\n");
    mysend(s_fd, buf, "MAIL FROM command failed!\n");
    myresv(s_fd);
    

    // TODO: Send RCPT TO command and print server response
    strcpy(buf, "RCPT TO:<");
    strcat(buf, receiver);
    strcat(buf, ">\r\n");
    mysend(s_fd, buf, "RCPT TO command failed!\n");
    myresv(s_fd);

    
    // TODO: Send DATA command and print server response
    mysend(s_fd, "DATA\r\n", "DATA command failed!\n");
    myresv(s_fd);

    // TODO: Send message data
    sprintf(buf, "From: %s\r\nTo: %s\r\nMIME-Version: 1.0\r\nContent-Type: multipart/mixed; boundary=qwertyuiopasdfghjklzxcvbnm\r\nSubject: ",from,receiver);
    strcat(buf, subject), strcat(buf, "\r\n\r\n");
    mysend(s_fd, buf, "Send message data header failed!\n");
    if (msg != NULL)
    {
        // 判断文件是否存在
        if( access(msg, F_OK)!=0 ){     // 不存在
            mysend(s_fd, msg, "Send message data(text file) failed!\n");
            mysend(s_fd, "\r\n\r\n", "Send message failed!\n");
        } else {                        // 存在
            sprintf(buf, "--qwertyuiopasdfghjklzxcvbnm\r\nContent-Type:text/plain\r\n\r\n");
            char *str = myfile2str(msg);
            strcat(buf, str);
            strcat(buf, "\r\n\r\n");
            mysend(s_fd, buf, "Send message data(attach file) failed!\n");
            free(str);
        }
    }
    if (att_path != NULL)
    {
        sprintf(buf, "--qwertyuiopasdfghjklzxcvbnm\r\nContent-Type:application/octet-stream;charset=\"utf-8\";name=\"");
        strcat(buf, att_path);
        strcat(buf, "\"\r\nContent-Transfer-Encoding: base64\r\nContent-Disposition: attachment;name=");
        strcat(buf, att_path);
        strcat(buf, "\r\n\r\n");
        mysend(s_fd, buf, "Send message data(attach file header) failed!\n");

        FILE *fp = fopen(att_path, "r");
        if (fp == NULL)
        {
            perror("file not exist");
            exit(EXIT_FAILURE);
        }
        FILE *fp_64 = fopen("tmp.encode", "w");  // 创建一个文件，用于存储编码后的文件
        encode_file(fp, fp_64);
        fclose(fp_64);
        char *str = myfile2str("tmp.encode");
        sprintf(buf, "%s",str);
        strcat(buf, "\r\n--qwertyuiopasdfghjklzxcvbnm\r\n");
        mysend(s_fd, buf, "Send message data(attach file) failed!\n");
        free(str);
    }
    

    // TODO: Message ends with a single period
    mysend(s_fd, end_msg, "Message end failed!\n");
    myresv(s_fd);

    // TODO: Send QUIT command and print server response
    mysend(s_fd, "quit\r\n", "QUIT command failed!\n");
    myresv(s_fd);

    close(s_fd);
}

int main(int argc, char* argv[])
{
    int opt;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}
