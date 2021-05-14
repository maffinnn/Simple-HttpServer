//web server -- epoll model
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
char *get_mime_type(char *name);
static ssize_t my_read(int fd, char *ptr);
size_t Readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; n++)
    {
        if ((rc = my_read(fd, &c)) == 1)
        {
            *ptr++ = c;
            if (c == '\n')
                break;
        }
        else if (rc == 0)
        {
            *ptr = 0;
            return n - 1;
        }
        else
            return -1;
    }
    *ptr = 0;

    return n;
}
void sighander(int signo)
{
    printf("signo==[%d]\n", signo);
}
int response_header(int, char *, char *, char *, int);
int response_file(int, char *);
int http_request(int cfd, int epfd)
{
    //读取请求行数据， 分析出要请求的资源文件名
    //read the request header, determine requested resources' file name
    int n;
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    n = Readline(cfd, buf, sizeof(buf));
    if (n <= 0)
    {
        //对方关闭链接对于server来说也属于读事件 内核有一个三次握手过程， 内核任然可以任然可以监控到读事件
        //n==0 client close connection no more data read 对方关闭连接
        //n<0 read error 读异常
        printf("read error or client connection closed\n");
        close(cfd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL); //remove fd from the tree
        return -1;
    }

    printf("buf==[%s]\n", buf);
    //1. GET/POST request; 2.file name; 3.protocol
    //e.g. GET /hanzi.c HTTP/1.1
    char reqType[16] = {0};
    char fileName[256] = {0};
    char protocalVersion[16] = {0};
    sscanf(buf, "%[^ ] %[^ ] %[^\r\n]", reqType, fileName, protocalVersion); //in between %[^ ] need to make a space
    printf("%s\n", reqType);
    printf("%s\n", fileName);
    printf("%s\n", protocalVersion);

    char *pFILE = fileName;
    if (strlen(fileName) <= 1)
    { //if visiting the current process working directory
        strcpy(pFILE, "./");
    }
    else
    {
        pFILE = fileName + 1; //remove the '\' character
    }
    //loop to read the rest of the data 避免数据产生粘包
    //note that Readline function is a blocking function need to set fd as non blocking
    while ((n = Readline(cfd, buf, sizeof(buf))) > 0)
    {
        ;
    } //将cfd非阻塞，否则循环读完剩下的头部请求内容后read函数继续阻塞导    致下一步无法进行

    //determine whether the file exits
    struct stat st;
    //check file status
    if (stat(pFILE, &st) < 0)
    {

        //if the requested file is not found
        printf("File Not Found\n");
        //send response header
        response_header(cfd, "404", "NOT FOUND", get_mime_type(".html"), 0);
        //send the retrieved file content
        response_file(cfd, "error.html");
    }
    else
    {

        //if file found
        //determine file type
        if (S_ISREG(st.st_mode))
        {
            //usual file just need to send the content

            //send the header
            response_header(cfd, "200", "OK", get_mime_type(pFILE), st.st_size);
            //send the file content
            response_file(cfd, pFILE);
        }
        else if (S_ISDIR(st.st_mode))
        {

            //dirctory file
            //int scandir(const char* dirp, struct dirent ***namelist, int (*filter)(const struct dirent *), int (*c    ompar)(*const struct dirent **, const struct dirent **b)
            //comapr->alphasort or versionsort
            printf("directory file\n");
            //http header
            //目录信息存在html文件里
            response_header(cfd, "200", "OK", get_mime_type(".html"), 0);
            //send html header
            //发html文件时头部和尾部是不变的 发送的时候只要发个头部和尾部然后中间组装起来便可
            response_file(cfd, "html/dir_header.html");

            //send file directory list
            char buffer[1024];
            struct dirent **namelist;
            int num;
            //返回读到的目录文件个数
            num = scandir(pFILE, &namelist, NULL, alphasort);
            if (num < 0)
            {
                perror("scandir error");
                close(cfd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
                //process should not exit
                return -1;
            }
            else
            {

                while (num--)
                {

                    printf("%s\n", namelist[num]->d_name);
                    memset(buffer, 0, sizeof(buffer));
                    if (namelist[num]->d_type == DT_DIR)
                    {
                        sprintf(buffer, "<li><a href=%s/> %s </a></li>", namelist[num]->d_name, namelist[num]->d_nam e);
                    }
                    else
                    {
                        //regular file
                        sprintf(buffer, "<li><a href=%s> %s </a></li>", namelist[num]->d_name, namelist[num]->d_name);
                    }
                    free(namelist[num]);
                    write(cfd, buffer, strlen(buffer));
                }

                free(namelist);
            }

            //send html tail
            response_file(cfd, "html/dir_tail.html");
        }
    }
}
//HTTP/1.1 200 OK
int response_header(int cfd, char *status_code, char *msg, char *fileType, int contentLength)
{
    char buf[1024] = {0};
    //HTTP/1.1 200 OK状态行
    sprintf(buf, "HTTP/1.1 %s %s\r\n", status_code, msg);
    memset(buffer, 0, sizeof(buffer));
    if (namelist[num]->d_type == DT_DIR)
    {
        sprintf(buffer, "<li><a href=%s/> %s </a></li>", namelist[num]->d_name, namelist[num]->d_nam e);
    }
    else
    {
        //regular file
        sprintf(buffer, "<li><a href=%s> %s </a></li>", namelist[num]->d_name, namelist[num]->d_name);
    }
    free(namelist[num]);
    write(cfd, buffer, strlen(buffer));
}

free(namelist);
}

//send html tail
response_file(cfd, "html/dir_tail.html");
}
}
}
//HTTP/1.1 200 OK
int response_header(int cfd, char *status_code, char *msg, char *fileType, int contentLength)
{
    char buf[1024] = {0};
    //HTTP/1.1 200 OK状态行
    sprintf(buf, "HTTP/1.1 %s %s\r\n", status_code, msg);
    //content type
    sprintf(buf + strlen(buf), "Content-Type:%s\r\n", fileType);
    //要么写准确的要么不写
    if (contentLength > 0)
    {
        sprintf(buf + strlen(buf), "Content-Length:%d\r\n", contentLength);
    }
    strcat(buf, "\r\n");
    //以上把文件头部信息已经拼好了
    write(cfd, buf, strlen(buf));

    return 0;
}

int response_file(int cfd, char *fileName)
{
    //open the file
    int fd = open(fileName, O_RDONLY);
    if (fd < 0)
    {
        printf("open error\n");
        return -1;
    }

    //loop to read 循环读文件
    char buf[1024];
    int n;
    while (1)
    {

        memset(buf, 0, sizeof(buf));

        n = read(fd, buf, sizeof(buf));
        if (n <= 0)
        {
            break;
        }
        else
        {
            write(cfd, buf, n);
        }
    }
    return 0;
}

int main()
{

    //若web服务器给浏览器发送数据时， 浏览器已关闭链接
    //则web服务器就会收到sigpipe信号 默认动作为终止进程
    //此处直接忽略不处理
    signal(SIGPIPE, SIG_IGN);

    //改变当前的工作目录这样使得requested file文件和此进程是在相同目录下
    //change current process working directory so as to have the same working dirctiory as the requested file
    char path[256] = {0};
    //getenv() to get the desired working dirctory 获取环境变量的值
    //这个函数可以通过这个环境变量的值名字把这个环境变量的值打出来
    sprintf(path, "%s", getenv("HOME")); //用户家目录
    chdir(path);

    //create socket
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket error\n");
    }

    char *IP = NULL;
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    if (IP == NULL)
    {
        //if ip == 0.0.0.0 means any ip can connect;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if (inet_pton(AF_INET, IP, &serv_addr.sin_addr.s_addr))
        {
            perror(IP); //convert failed
            exit(1);
        }
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888);

    //set port reusable;
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int ret = bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret == -1)
    {
        perror("bind error\n");
    }

    listen(lfd, 5);

    //create an epoll tree
    int epfd = epoll_create(1024); //the argument is only required to be greater than 0 will do
    if (epfd < 0)
    {
        perror("epoll_create error\n");
        close(lfd);
        return -1;
    }

    //add the fd to the tree
    struct epoll_event ev;
    ev.data.fd = lfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

    int nready;
    int sockfd;
    struct epoll_event events[1024];
    while (1)
    {
        //wait for occurrence of events
        nready = epoll_wait(epfd, events, 1024, -1); //-1 means constantly blocking; 0 means non-blocking; >0 means t    he blocking duration;
        if (nready < 0)
        {
            if (errno == EINTR)
                continue;
            else
                break;
        }

        for (int i = 0; i < nready; i++)
        {

            sockfd = events[i].data.fd;

            // if there are connection requests from client
            if (sockfd == lfd)
            {

                //accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
                int cfd = accept(lfd, NULL, NULL);
                if (cfd < 0)
                {
                    perror("accept error");
                }

                //set fd so that Readline() to be non-blocking
                int flag = fcntl(cfd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(cfd, F_SETFL, flag);

                //add new event to tree
                ev.data.fd = cfd;
                ev.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
            }
            else
            {
                //there are data sent from client
                http_request(sockfd, epfd);
            }
        }
    }

    return 0;
}
static ssize_t my_read(int fd, char *ptr)
{
    static int read_cnt;
    static char *read_ptr;
    static char read_buf[100];

    if (read_cnt <= 0)
    {
    again:
        if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0)
        {
            if (errno == EINTR)
                goto again;
            return -1;
        }
        else if (read_cnt == 0)
            return 0;
        read_ptr = read_buf;
    }
    read_cnt--;
    *ptr = *read_ptr++;

    return 1;
}
char *get_mime_type(char *name)
{
    char *dot;

    dot = strrchr(name, '.');
    if (dot == (char *)0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}
ssize_t Writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0)
    {
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}
