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

int main(int argc, char** agrv){
    int l_fd, c_fd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    // rio_t rio; //일회성 에코
    // char test[MAXLINE], test_buff[MAXLINE]; // 일회성 에코

    l_fd = open_listenfd(agrv[1]);

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
    exit(0);
}
void doit(int fd){
    signal(SIGPIPE, SIG_IGN);
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
//////////////////////////////////////////////////////////////////////
// void serve_static(int fd, char* fname, int fsize) {
//     int src_fd, n;
//     char* src_p, ftype[MAXLINE], b[MAXLINE];
//     int start = 0, end = fsize - 1;
//     char *range_header = getenv("HTTP_RANGE");  // Range 헤더 가져오기

//     // 파일 타입 가져오기
//     get_filetype(fname, ftype);

//     // Range 헤더가 있는지 확인
//     if (range_header) {
//         // Range 요청이 있을 경우 처리
//         if (sscanf(range_header, "bytes=%d-%d", &start, &end) == 2) {
//             if (end == -1) end = fsize - 1;  // end가 없을 경우 파일 끝까지 전송
//         } else if (sscanf(range_header, "bytes=%d-", &start) == 1) {
//             end = fsize - 1;  // start만 있을 경우 파일 끝까지 전송
//         } else {
//             // 잘못된 Range 요청이면 전체 파일 전송
//             start = 0;
//             end = fsize - 1;
//         }
//     }

//     // 응답 헤더 작성
//     if (range_header) {
//         sprintf(b, "HTTP/1.1 206 Partial Content\r\n");
//         sprintf(b, "%sContent-Range: bytes %d-%d/%d\r\n", b, start, end, fsize);
//     } else {
//         sprintf(b, "HTTP/1.1 200 OK\r\n");
//     }

//     sprintf(b, "%sServer: Tiny Web Server\r\n", b);
//     sprintf(b, "%sConnection: keep-alive\r\n", b);
//     sprintf(b, "%sContent-length: %d\r\n", b, end - start + 1);
//     sprintf(b, "%sContent-type: %s\r\n\r\n", b, ftype);

//     // 응답 헤더 전송
//     rio_writen(fd, b, strlen(b));

//     // 요청한 범위만큼 파일 전송
//     printf("Response headers:\n");
//     printf("%s", b);

//     if (getenv("HHEAD")) {  // HHEAD 환경 변수 체크
//         unsetenv("HHEAD");
//         return;
//     }

//     // 파일을 읽어서 클라이언트에게 전송
//     src_fd = open(fname, O_RDONLY, 0);
//     if (src_fd < 0) {
//         perror("open");
//         return;
//     }

//     // 요청된 범위만큼 파일을 전송
//     src_p = Mmap(0, fsize, PROT_READ, MAP_PRIVATE, src_fd, 0);
//     Rio_writen(fd, src_p + start, end - start + 1);  // 요청된 범위만큼 전송

//     close(src_fd);
//     Munmap(src_p, fsize);
// }
//////////////////////////////////////////////////////////

void serve_static(int fd, char* fname, int fsize){

    int src_fd, n;
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
        unsetenv("HHEAD");
        return;
    }
    
    src_fd = open(fname, O_RDONLY, 0);
    src_p = Mmap(0, fsize, PROT_READ, MAP_PRIVATE, src_fd, 0);
    Rio_writen(fd, src_p, fsize);

    close(src_fd);
    Munmap(src_p, fsize);

// 파일 크기만큼 메모리 할당
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
    unsetenv("HHEAD");
}


///////////////////////////////////////////////////////////////////////////////////
// /* $begin tinymain */
// /*
//  * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
//  *     GET method to serve static and dynamic content.
//  *
//  * Updated 11/2019 droh
//  *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
//  */
// #include <signal.h>
// #include "csapp.h"

// void doit(int fd);
// void read_requesthdrs(rio_t *rp);
// int parse_uri(char *uri, char *filename, char *cgiargs);
// void serve_static(int fd, char *filename, int filesize, char *method);
// void get_filetype(char *filename, char *filetype);
// void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
// void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

// int main(int argc, char **argv)
// {
//     int listenfd, connfd;
//     char hostname[MAXLINE], port[MAXLINE];
//     socklen_t clientlen;
//     struct sockaddr_storage clientaddr;

//     /* Check command line args */
//     if (argc != 2)
//     {
//         fprintf(stderr, "usage: %s <port>\n", argv[0]);
//         exit(1);
//     }

//     listenfd = Open_listenfd(argv[1]); // 전달받은 포트 번호를 사용해 수신 소켓 생성
//     while (1)
//     {
//         clientlen = sizeof(clientaddr);
//         connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);                       // line:netp:tiny:accept 클라이언트 연결 요청 수신
//         Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // 클라이언트의 호스트 이름과 포트 번호 파악
//         printf("Accepted connection from (%s, %s)\n", hostname, port);
//         doit(connfd);  // line:netp:tiny:doit 클라이언트의 요청 처리
//         Close(connfd); // line:netp:tiny:close 연결 종료
//     }
// }

// void doit(int fd)
// {
//     signal(SIGPIPE, SIG_IGN);

//     int is_static;
//     struct stat sbuf;
//     char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // MAXLINE: 8192
//     char filename[MAXLINE], cgiargs[MAXLINE];
//     rio_t rio; // 버퍼

//     /* 요청 라인과 헤더 읽기 */
//     Rio_readinitb(&rio, fd);           // 버퍼(rio)를 초기화하고, rio와 파일 디스크립터(fd)를 연결한다.
//     Rio_readlineb(&rio, buf, MAXLINE); // 버퍼(rio)에서 한줄씩 읽어서 buf에 저장한다. MAXLINE보다 길면 자동으로 버퍼를 증가시켜서 /r과 /n 문자를 모두 읽을 때까지 계속 읽어들인다.
//     printf("Request headers:\n");
//     printf("%s", buf);
//     sscanf(buf, "%s %s %s", method, uri, version); // buf 문자열에서 method, uri, version을 읽어온다.
//     if (strcasecmp(method, "GET"))
//     { // 요청 메소드가 GET이 아니면 에러 처리
//         clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
//         return;
//     }
//     read_requesthdrs(&rio); // 요청 헤더를 읽고 파싱한다.(요청 헤더를 읽어서 클라이언트가 요청한 추가 정보를 처리한다.)

//     /* URI 파싱 */
//     is_static = parse_uri(uri, filename, cgiargs); // 요청이 정적 콘텐츠인지 동적 콘텐츠인지 파악한다.
//     if (stat(filename, &sbuf) < 0)
//     { // 파일이 디스크에 없으면 에러 처리
//         clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
//         return;
//     }

//     if (is_static)
//     { // 정적 컨텐츠인 경우
//         if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
//         { // 일반 파일이 아니거나 읽기 권한이 없는 경우 에러 처리
//             clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
//             return;
//         }
//         serve_static(fd, filename, sbuf.st_size, method); // 정적 컨텐츠 제공
//     }
//     else
//     { // 동적 컨텐츠인 경우
//         if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
//         { // 일반 파일이 아니거나 실행 권한이 없는 경우 에러 처리
//             clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
//             return;
//         }
//         serve_dynamic(fd, filename, cgiargs, method); // 동적 컨텐츠 제공
//     }
// }

// // 클라이언트에 에러를 전송하는 함수(cause: 오류 원인, errnum: 오류 번호, shortmsg: 짧은 오류 메시지, longmsg: 긴 오류 메시지)
// void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
// {
//     char buf[MAXLINE], body[MAXBUF]; // buf: HTTP 응답 헤더, body: HTML 응답의 본문인 문자열(오류 메시지와 함께 HTML 형태로 클라이언트에게 보여짐)

//     /* 응답 본문 생성 */
//     sprintf(body, "<html><title>Tiny Error</title>");
//     sprintf(body, "%s<body bgcolor="
//                   "ffffff"
//                   ">\r\n",
//             body);
//     sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
//     sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
//     sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

//     /* 응답 출력 */
//     sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
//     Rio_writen(fd, buf, strlen(buf)); // 클라이언트에 전송 '버전 에러코드 에러메시지'
//     sprintf(buf, "Content-type: text/html\r\n");
//     Rio_writen(fd, buf, strlen(buf));                              // 컨텐츠 타입
//     sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // \r\n: 헤더와 바디를 나누는 개행
//     Rio_writen(fd, buf, strlen(buf));                              // 컨텐츠 크기
//     Rio_writen(fd, body, strlen(body));                            // 응답 본문(HTML 형식)
// }

// void read_requesthdrs(rio_t *rp)
// {
//     char buf[MAXLINE];

//     Rio_readlineb(rp, buf, MAXLINE); // 요청 메시지의 첫번째 줄 읽기
//     printf("%s", buf);               // 헤더 필드 출력
//     while (strcmp(buf, "\r\n"))      // 버퍼에서 읽은 줄이 '\r\n'이 아닐 때까지 반복 (strcmp: 두 인자가 같으면 0 반환)
//     {
//         Rio_readlineb(rp, buf, MAXLINE);
//         printf("%s", buf); // 헤더 필드 출력
//     }
//     return;
// }

// // URI에서 요청된 파일 이름과 CGI 인자 추출
// int parse_uri(char *uri, char *filename, char *cgiargs)
// {
//     char *ptr;

//     if (!strstr(uri, "cgi-bin"))
//     { // uri에 cgi-bin이 없으면 정적 컨텐츠
//         strcpy(cgiargs, "");
//         strcpy(filename, ".");             // filename에 현재 디렉터리를 넣고
//         strcat(filename, uri);             // filename에 uri를 이어 붙인다.
//         if (uri[strlen(uri) - 1] == '/')   // uri가 /로 끝나면
//             strcat(filename, "home.html"); //  filename에 home.html를 이어 붙인다.
//         return 1;                          // 정적 컨텐츠일 때는 1 리턴
//     }
//     else
//     { // 동적 컨텐츠
//         ptr = index(uri, '?');
//         if (ptr)
//         {
//             strcpy(cgiargs, ptr + 1);
//             *ptr = '\0';
//         }
//         else
//             strcpy(cgiargs, "");
//         strcpy(filename, ".");
//         strcat(filename, uri);
//         return 0;
//     }
// }

// void serve_static(int fd, char *filename, int filesize, char *method)
// {
//     int srcfd;
//     char *srcp, filetype[MAXLINE], buf[MAXBUF];
//     rio_t rio;
//     /* 응답 헤더 전송 */
//     get_filetype(filename, filetype);                          // 파일 타입 결정
//     sprintf(buf, "HTTP/1.0 200 OK\r\n");                       // 상태 코드
//     sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);        // 서버 이름
//     sprintf(buf, "%sConnection: close\r\n", buf);              // 연결 방식
//     sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);   // 컨텐츠 길이
//     sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); // 컨텐츠 타입
//     Rio_writen(fd, buf, strlen(buf));                          // buf에서 fd로 전송(헤더 정보 전송)
//     printf("Response headers:\n");
//     printf("%s", buf);

//     /* HTTP HEAD 메소드 처리 */
//     if (strcasecmp(method, "HEAD") == 0)
//     {
//         return; // 응답 바디를 전송하지 않음
//     }

//     /* 응답 바디 전송 */
//     srcfd = Open(filename, O_RDONLY, 0); // 파일 열기
//     // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일을 가상메모리에 매핑
//     srcp = malloc(filesize);          // 파일을 위한 메모리 할당
//     Rio_readinitb(&rio, srcfd);       // 파일을 버퍼로 읽기위한 초기화
//     Rio_readn(srcfd, srcp, filesize); // 읽기
//     Close(srcfd);                     // 파일 디스크립터 닫기
//     Rio_writen(fd, srcp, filesize);   // 파일 내용을 클라이언트에게 전송 (응답 바디 전송)
//     free(srcp);                       // 매핑된 가상메모리 해제
// }

// void get_filetype(char *filename, char *filetype)
// {
//     if (strstr(filename, ".html"))
//         strcpy(filetype, "text/html");
//     else if (strstr(filename, ".gif"))
//         strcpy(filetype, "image/gif");
//     else if (strstr(filename, ".png"))
//         strcpy(filetype, "image/png");
//     else if (strstr(filename, ".jpg"))
//         strcpy(filetype, "image/jpg");
//     else if (strstr(filename, ".mp4"))
//         strcpy(filetype, "video/mp4");
//     else
//         strcpy(filetype, "text/plain");
// }

// void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
// {
//     char buf[MAXLINE], *emptylist[] = {NULL};

//     sprintf(buf, "HTTP/1.0 200 OK\r\n");
//     Rio_writen(fd, buf, strlen(buf));
//     sprintf(buf, "Server: Tiny Web Server\r\n");
//     Rio_writen(fd, buf, strlen(buf));

//     if (Fork() == 0) // 자식 프로세스 포크
//     {
//         setenv("QUERY_STRING", cgiargs, 1);   // QUERY_STRING 환경 변수를 URI에서 추출한 CGI 인수로 설정
//         setenv("REQUEST_METHOD", method, 1);  // QUERY_STRING 환경 변수를 URI에서 추출한 CGI 인수로 설정
//         Dup2(fd, STDOUT_FILENO);              // 자식 프로세스의 표준 출력을 클라이언트 소켓에 연결된 파일 디스크립터로 변경
//         Execve(filename, emptylist, environ); // 현재 프로세스의 이미지를 filename 프로그램으로 대체
//     }
//     Wait(NULL);
// }