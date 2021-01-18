#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <termio.h>
#include "rsh_client.h"
struct termios original_termios;
int tty_set_raw(int fd, struct termios *prevTermios)
{
    struct termios t;

    if (tcgetattr(fd, &t) == -1)
        return -1;

    if (prevTermios != NULL)
        *prevTermios = t;

    t.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
                        /* Noncanonical mode, disable signals, extended
                           input processing, and echoing */

    t.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR |
                      INPCK | ISTRIP | IXON | PARMRK);
                        /* Disable special handling of CR, NL, and BREAK.
                           No 8th-bit stripping or parity error handling.
                           Disable START/STOP output flow control. */

    t.c_oflag &= ~OPOST;                /* Disable all output processing */

    t.c_cc[VMIN] = 1;                   /* Character-at-a-time input */
    t.c_cc[VTIME] = 0;                  /* with blocking */

    if (tcsetattr(fd, TCSAFLUSH, &t) == -1)
        return -1;

    return 0;
}
void reduct_termios(void){
    tcsetattr(STDIN_FILENO,TCSANOW,&original_termios);
}

int connect2server(char *addr,int port){
    struct sockaddr_in client = {0};
    int client_fd;
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = inet_addr(addr);
    client.sin_port = htons(port);
    client_fd = socket(AF_INET,SOCK_STREAM,0);
    if(connect(client_fd,(struct sockaddr *)&client,sizeof(client)) == -1){
        perror("连接服务器失败");
        return -1;
    }
    return client_fd;
}
int main(int argc,char *argv[]){
    int client_fd;
    fd_set in_fds;
    int n_read;
    char *buf = (char *)malloc(BUF_SIZE);
    char *addr = malloc(100);
    int port;
    struct termios modified_termios;

    if(argc == 1){
        // printf("Usage: ./client [ip] [port]\n");
        // printf("连接到127.0.0.1:8082\n");
        addr = DEFAULT_ADDR;
        port = DEFAULT_PORT;
    }else if(argc == 2){
        addr = argv[1];
        port = DEFAULT_PORT;
    }else if(argc == 3){
        addr = argv[1];
        port = atoi(argv[2]);
        
    }
    //客户端和服务器端选择关闭一个回显即可 这里选择关闭客户端回显 然后在客户端退出之后 再次打开回显
    tcgetattr(STDIN_FILENO,&original_termios);
    atexit(reduct_termios);
    tty_set_raw(STDIN_FILENO,&modified_termios);

    client_fd = connect2server(addr,port);


    while(1){
        
        FD_ZERO(&in_fds);
        FD_SET(client_fd,&in_fds);
        FD_SET(STDIN_FILENO,&in_fds);


        if(select(10,&in_fds,NULL,NULL,NULL) == -1){
            perror("select");
            exit(-1);
        }
        if(FD_ISSET(client_fd,&in_fds) == 1){
            n_read= read(client_fd,buf,BUF_SIZE);
            if(n_read <= 0){
                //perror("read client fd\n");
                exit(-1);
            }
            write(STDOUT_FILENO,buf,n_read);
        }
        if(FD_ISSET(STDIN_FILENO,&in_fds) == 1){
            n_read = read(STDIN_FILENO,buf,BUF_SIZE);
            if(n_read <= 0){
                //perror("read STDIN");
                exit(-1);
            }
            write(client_fd,buf,n_read);
        }

    }

    return 0;
}