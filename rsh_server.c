#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termio.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "rsh_server.h"
#define BUF_SIZE 1000
int server_listen(int port){
    struct sockaddr_in server = {0};
    server.sin_family =  AF_INET;
    server.sin_addr.s_addr = inet_addr(LISTEN_ADDR);
    if(port == 0){
        server.sin_port = htons(LISTEN_PORT);
    }else{
        server.sin_port = htons(port);
    }
    int server_fd = -1;
    if( (server_fd = socket(AF_INET,SOCK_STREAM,0)) == -1){
        perror("socket create");
        exit(-1);
    }
    printf("socket create ok\n");
    fflush(NULL);

    if(bind(server_fd,(struct sockaddr *)&server,sizeof(server)) == -1){
        perror("bind");
        exit(-1);
    }
    printf("socket bind ok\n");
    fflush(NULL);
    
    if(listen(server_fd,4) == -1){
        perror("listen");
        exit(-1);
    }
    printf("listen ok\n");
    fflush(NULL);
    return server_fd;
}
int accept_request(int server_fd){
    struct sockaddr_in tmp_sock = {0};
    tmp_sock.sin_family = AF_INET;
    tmp_sock.sin_port = 0; //accept设定随机端口
    tmp_sock.sin_addr.s_addr = inet_addr(LISTEN_ADDR);
    socklen_t tmp_sock_len = sizeof(tmp_sock);
    int client_fd = accept(server_fd,(struct sockaddr*)&tmp_sock,&tmp_sock_len);
    if(client_fd == -1){
        printf("accept error\n");
        exit(-1);
    }
    printf("accept ok\n");
    fflush(NULL);
    return client_fd;
}
int process_once_request(){

}

int pty_master_open(char *slave_name){
    int mfd = posix_openpt(O_RDWR | O_NOCTTY);
    //grantpt(mfd);
    unlockpt(mfd);
    char *tmp_name = ptsname(mfd);
    strncpy(slave_name,tmp_name,strlen(tmp_name));
    return mfd;
}
//创建子进程 复制文件描述符等





pid_t pty_fork(int *masterfd){
    char slave_name[100];
    int mfd = -1;
    int slavefd = -1;
    pid_t child_pid;
    if((mfd = pty_master_open(slave_name)) == -1){
        return -1;
    }

    child_pid = fork();
    if(child_pid == -1){
        return -1;
    }
    
    if(child_pid == 0){
        setsid();
        close(mfd);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        //printf("%s\n",slave_name);
        if((slavefd = open(slave_name, O_RDWR)) == -1){     //开一个控制终端
            return -1;
        }
        dup2(slavefd, STDOUT_FILENO);
        dup2(slavefd, STDERR_FILENO);
        if(execlp("/usr/bin/bash","/usr/bin/bash","-p",(char *)NULL) == -1){
            perror("execve");
            exit(-1);
        }
        return child_pid;
    }

    if(child_pid > 0){
        *masterfd = mfd;
        return child_pid;
    }

}


pid_t daemon_fork(char *logfile){
    pid_t child_pid;
    int logfd;
    child_pid = fork();//fork第二个子进程 为了setsid
    switch(child_pid){
        case 0 :
            break;
        case -1 :
            exit(0);
        default :
            exit(EXIT_SUCCESS);//主进程退出
    }

    setsid();
    umask(0);
    //chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    logfd = open(logfile,O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR);
    dup2(logfd,STDOUT_FILENO);
    dup2(logfd,STDERR_FILENO);
    return child_pid;
}


int main(int argc,char *argv[]){
    int server_fd,client_fd;
    int mfd;
    int stat;
    int daemon_stat;
    char buf[1000];
    int tmpfd;
    int logfd;
    int n_read;
    fd_set in_fds;
    char *logfile = "./rshlog";
    char *scriptfile= "./tmp";
    int daemon_child_pid ;
    int shell_child_pid;
    int port;

    if(argc == 1){
        port = 0;
    }else if(argc == 2){
        port = atoi(argv[1]);
    }else{
        printf("Usage: ./rsh_server [port]\n");
    }

    daemon_fork(logfile);
    server_fd = server_listen(port);
    while (1){
        shell_child_pid = pty_fork(&mfd);
        client_fd = accept_request(server_fd);
        while(1){
            if(waitpid(shell_child_pid,&stat,WNOHANG) > 0 ){
                break;  //如果shell子进程死了退出循环(客户端执行了exit)
            }
            FD_ZERO(&in_fds);
            FD_SET(client_fd,&in_fds);
            FD_SET(mfd,&in_fds);
            
            if(select(10,&in_fds,NULL,NULL,NULL) == -1){
                break;
            }
            if(FD_ISSET(client_fd,&in_fds) == 1){ //套接字检查到输入 也就是命令的输入
                n_read = read(client_fd,buf,BUF_SIZE);
                if(n_read <= 0){
                   break;
                }
                write(mfd,buf,n_read);
            }


            if(FD_ISSET(mfd,&in_fds) == 1){ //mfd上检查到了输入 ;也就是程序的输出
                n_read = read(mfd,buf,BUF_SIZE);
                if(n_read <= 0){
                    break;
                }
                write(client_fd,buf,n_read);
            }
        }
        close(client_fd);//关闭2端套接字

    }
    
    
    
    
    

   
    

   
    return 0;
}