#include <stdio.h>
#include "csapp.h"
#include <signal.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
//
int parser(char *urL, char *doM, char *urI, char* porT, char* htT);
//
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **agrv)
{
  signal(SIGPIPE, SIG_IGN);
  int c_l_fd, c_fd;
  char hname[MAXLINE], hport[MAXLINE];
  socklen_t clilen;
  struct sockaddr cliaddr;
  printf("%s", user_agent_hdr);

  c_l_fd = Open_listenfd(agrv[1]);
  while (1)
  {

    clilen = sizeof(cliaddr);
    c_fd = Accept(c_l_fd, (SA *)&cliaddr, &clilen);
    Getnameinfo((SA *)&cliaddr, clilen, hname, MAXLINE, hport, MAXLINE, 0);
    printf("..connecting..\nhostname:%s\nconnect port: %s\n", hname, hport);
    // rio_writen(c_fd, "..connected..\n", 14);

    doit(c_fd);
    
    close(c_fd);
  }
}

void doit(int c_fd)
{
  // signal(SIGPIPE, SIG_IGN);
  char buF[MAXLINE], meT[MAXLINE], urL[MAXLINE], veR[MAXLINE];
  char doM[MAXLINE], urI[MAXLINE], porT[MAXLINE], htT[MAXLINE];
  char head[MAXLINE], ptosF[MAXLINE], testbuF[MAXLINE];
  char ptosbuF[MAXLINE], stopbuF[MAXLINE];
  int s_fd;
  rio_t doitrio, serrio;


  
  Rio_readinitb(&doitrio, c_fd);
  Rio_readlineb(&doitrio, buF, MAXLINE);
  sscanf(buF, "%s %s %s", meT, urL, veR);
  printf("method: %s\nurl: %s\nversion: %s\n\n", meT, urL, veR);

  parser(urL, doM, urI, porT, htT);
  sprintf(ptosF, "%s %s %s\r\n", meT, urI, veR);
  sprintf(ptosF, "%sHost: %s\r\n", ptosF, doM);
  sprintf(ptosF, "%sProxy-Connection: close\r\n", ptosF);
  sprintf(ptosF, "%sUser-Agent: ProxyServer/1.0\r\n\r\n", ptosF);
  
  ///testtesttesttestestestestestesdtestes
  // sprintf(testbuF, "url: %s\r\n", urL);
  // sprintf(testbuF, "%s--parser--\r\n", testbuF);
  // sprintf(testbuF, "%shttp: %s\r\n", testbuF, htT);
  // sprintf(testbuF, "%sdomain: %s\r\n", testbuF, doM);
  // sprintf(testbuF, "%sport: %s\r\n", testbuF, porT);
  // sprintf(testbuF, "%suri: %s\r\n", testbuF, urI);

  // sprintf(head, "HTTP/1.1 200 OK\r\n");
  // Rio_writen(c_fd, head, strlen(head));
  // sprintf(head, "content-type: text/html\r\n");
  // Rio_writen(c_fd, head, strlen(head));
  // sprintf(head, "content-length: %d\r\n\r\n", (int)strlen(testbuF));
  // Rio_writen(c_fd, head, strlen(head));

  // Rio_writen(c_fd, testbuF, strlen(testbuF));
  // Rio_writen(c_fd, ptosF, strlen(ptosF));
  ///testtesttesttestestestestestesdtestes
  // printf("1Request sent to Tiny server:\n%s\n", ptosF);
  s_fd = Open_clientfd(doM, porT);

  Rio_readinitb(&serrio, s_fd);
  Rio_writen(s_fd, ptosF, strlen(ptosF));
  int n;
  while ((n = Rio_readlineb(&serrio, stopbuF, MAXLINE)) > 0) {
    printf("%s\n", stopbuF);
    Rio_writen(c_fd, stopbuF, n);
  }
  close(s_fd);
}

int parser(char *urL, char *doM, char *urI, char* porT, char* htT)
{
  char *start, *end;
  char temp_url[200];

  start = strstr(urL, "://");
  if (start != NULL) {

      strncpy(htT, urL, start - urL);
      htT[start - urL] = '\0';
      start += 3;

  } else {

      strcpy(htT, "http");
      start = urL;

  }

  // 포트번호 확인을 위해 호스트/포트 구분
  end = strchr(start, '/');
  if (end != NULL) {

      strncpy(temp_url, start, end - start);
      temp_url[end - start] = '\0';
      strcpy(urI, end);

  } else {

      strcpy(temp_url, start);
      strcpy(urI, "/");

  }
  start = strchr(temp_url, ':');
  if (start != NULL) {

      strncpy(doM, temp_url, start - temp_url);
      doM[start - temp_url] = '\0';
      strcpy(porT, start + 1);

  } else {

      strcpy(doM, temp_url);
      strcpy(porT, "80");
  }
}