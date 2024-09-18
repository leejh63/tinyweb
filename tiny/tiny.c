#include "csapp.h"
#include <strings.h>
#include <signal.h>
////
#define PORTNUM "5125"
#define SERVER_ADDR "43.203.26.16"
typedef struct sockaddr SA;

////

////
void doit(int fd);
void c_error(int fd, char* m, char* ernum, char* smsg, char* lmsg);
int p_uri(char* uri, char* fname, char* cgi);
void serve_static(int fd, char* fname, int fsize);
void r_rqhs(rio_t *rio);
void serve_dynamic(int fd, char* fname, char* cgi);
void get_filetype(char* fname, char* ftype);
////

int main(void){
    signal(SIGPIPE, SIG_IGN);
    int l_fd, c_fd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    // rio_t rio; //일회성 에코
    // char test[MAXLINE], test_buff[MAXLINE]; // 일회성 에코

    l_fd = open_listenfd(PORTNUM);

    while(1){

        clientlen = sizeof(clientaddr);
        c_fd = accept(l_fd, (SA*)&clientaddr, &clientlen);
        getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

        // 일회성 에코
        // printf("..connect checking..\nhostname:%s\nconnect port: %s\n", hostname, port);
        // Rio_readinitb(&rio, c_fd);                      // 클라 쪽 데이터 > rio구조체로
        // sprintf(test,"..connect checking..\n");
        // rio_writen(c_fd, test, strlen(test));           // test에 들어간 데이터 > 클라 쪽으로
        // rio_readlineb(&rio, test_buff, MAXLINE);        // rio구조체 데이터 > test_buff 쪽으로
        // rio_writen(c_fd, test_buff, strlen(test_buff)); // test_buff 데이터 > 클라 쪽으로
        // rio_writen(c_fd, "----typing request----\n", 24);

        // 서버 쪽 처리함수
        doit(c_fd);

        // 연결 끊기
        close(c_fd);
    }
    return 0;
}
void doit(int fd){

    int is_static;
    struct stat sbuf;
    char b[MAXLINE], m[MAXLINE], uri[MAXLINE], v[MAXLINE];
    char fname[MAXLINE], cgi[MAXLINE];
    rio_t doitrio;

    rio_readinitb(&doitrio, fd);
    printf("waiting for request\n");
    rio_readlineb(&doitrio, b, MAXLINE);
    printf("Requested Header :\n");
    sscanf(b, "%s %s %s", m, uri, v);
    printf("method: %s\nuri: %s\nversion: %s\n", m, uri, v);



    if (!(strcasecmp(m, "GET") ^ strcasecmp(m, "HHEAD"))){// get일 경우 0 반환
        c_error(fd, m, "501", "NOT implemented", "Tiny does not implement this method");
        return;
    }

    if (!strcasecmp(m,"HHEAD")){
        setenv("HHEAD", "HHEAD", 1);
    }

    r_rqhs(&doitrio);

    is_static = p_uri(uri, fname, cgi);
    if (stat(fname, &sbuf) < 0){
        c_error(fd, fname, "404", "Not found","Tiny couldn't find this file");
        return;
    }

    if (is_static){ // 정적

        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
            c_error(fd, fname, "403", "Forbidden", "Tiny couldn't read this file");
            return;
        }

        serve_static(fd, fname, sbuf.st_size);

    }else{

        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
            c_error(fd, fname, "403", "Forbidden", "Tiny couldn't run the CGI program");
            return;
        }

        serve_dynamic(fd, fname, cgi);
    }
}
void c_error(int fd, char* m, char* ernum, char* smsg, char* lmsg){

    char head[MAXLINE], body[MAXLINE];

    // body build
    sprintf(body, "<html><title>!error!</title>");
    sprintf(body, "%s<body style=\"background-color: #FFA500;\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, ernum, smsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, lmsg, m);
    sprintf(body, "%s<hr><em>TThhee TTIINNYY WWEEBB SSEERRVVEERR</em>\r\n", body);

    // head 입력 및 출력
    sprintf(head, "HTTP/1.1 %s %s\r\n", ernum, smsg);
    rio_writen(fd, head, strlen(head));
    sprintf(head, "content-type: text/html\r\n");
    rio_writen(fd, head, strlen(head));
    sprintf(head, "content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen(fd, head, strlen(head));

    // body 출력
    rio_writen(fd, body, strlen(body));
}

void r_rqhs(rio_t *rio){

    char b[MAXLINE];

    rio_readlineb(rio, b, MAXLINE);
    while(strcmp(b, "\r\n")){
        rio_readlineb(rio, b, MAXLINE);
        printf("%s", b);
    }
    return;
}

int p_uri(char* uri, char* fname, char* cgi){

    char *ptr;

    if (!strstr(uri, "cgi-bin")){ // 정적 콘텐츠
        strcpy(cgi, "");
        strcpy(fname, ".");
        strcat(fname, uri);
        
        if (uri[strlen(uri)-1] == '/'){
            strcat(fname, "home.html");
        }

        return 1;
    }
    else{ // 동적 콘텐츠
        ptr = strchr(uri, '?');

        if (ptr){
            strcpy(cgi, ptr+1);
            *ptr = '\0';
        }
        else{
            strcpy(cgi, "");
        }

        strcpy(fname, ".");
        strcat(fname, uri);
        return 0;
    }
}

void serve_static(int fd, char* fname, int fsize){

    int src_fd;
    char* src_p, ftype[MAXLINE], b[MAXLINE];

    get_filetype(fname, ftype);
    sprintf(b, "HTTP/1.1 200 OK\r\n");
    sprintf(b, "%sServer: Tiny Web Server\r\n", b);
    sprintf(b, "%sConnection: keep-alive\r\n", b);
    sprintf(b, "%sContent-length: %d\r\n", b, fsize);  
    sprintf(b, "%sContent-type: %s\r\n\r\n", b, ftype); 



    rio_writen(fd, b, strlen(b));
    printf("Response headers:\n");
    printf("%s", b);
    if (getenv("HHEAD")){// hhead일경우
        return;
    }
    
    src_fd = open(fname, O_RDONLY, 0);
    src_p = Mmap(0, fsize, PROT_READ, MAP_PRIVATE, src_fd, 0);
    Rio_writen(fd, src_p, fsize);
    close(src_fd);
    Munmap(src_p, fsize);
    
    // // 파일 크기만큼 메모리 할당
    // src_p = (char*)malloc(fsize);
    // if (src_p == NULL) {
    //     perror("malloc");
    //     close(src_fd);
    //     return;
    // }

    // // 파일 내용을 메모리에 읽어옴
    // if (read(src_fd, src_p, fsize) != fsize) {
    //     perror("read");
    //     free(src_p);
    //     close(src_fd);
    //     return;
    // }

    // close(src_fd);

    // // 클라이언트에게 파일 내용 전송
    // rio_writen(fd, src_p, fsize);

    // // 메모리 해제
    // free(src_p);
}


void get_filetype(char* fname, char* ftype){

    if (strstr(fname,".html")){
        strcpy(ftype, "text/html");

    }else if (strstr(fname,".gif")){
        strcpy(ftype, "image/gif");

    }else if (strstr(fname,".png")){
        strcpy(ftype, "image/png");

    }else if (strstr(fname,".jpg")){
        strcpy(ftype, "image/jpeg");

    }else if(strstr(fname,".mp4")){
        strcpy(ftype, "video/mp4");

    }else{
        strcpy(ftype, "text/plain");
    }
}

void serve_dynamic(int fd, char* fname, char* cgi){

    char b[MAXLINE], *elist[] = { NULL };

    sprintf(b, "HTTP/1.1 200 OK\r\n");
    Rio_writen(fd, b, strlen(b));
    sprintf(b, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, b, strlen(b));

    if (Fork() == 0) {
    // 자식 프로세스에서 CGI 실행
    setenv("QUERY_STRING", cgi, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(fname, elist, environ);
    // 만약 Execve가 실패했다면 로그 출력
    perror("Execve failed");
    }
    // 부모 프로세스에서 자식 프로세스가 정상적으로 실행되는지 확인
    wait(NULL);
}