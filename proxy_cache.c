#include <stdio.h>
#include "csapp.h"
#include <signal.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
//
int parser(char *urL, char *doM, char *urI, char* porT, char* htT);
void *thread_test(void *vargp);
//
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

//chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;  // 캐시 보호용 mutex

// 캐시 엔트리 구조체 정의
typedef struct {
    char url[MAXLINE];          // 요청된 URL
    char response[MAX_OBJECT_SIZE];  // 서버 응답 (캐시된 데이터)
    int response_size;          // 응답 데이터 크기
} cache_entry_t;

// 캐시 구조체 정의 (여러 캐시 항목을 저장)
#define CACHE_SIZE 10  // 캐시에 저장할 수 있는 최대 엔트리 수
cache_entry_t cache[CACHE_SIZE];  // 캐시 배열
int cache_count = 0;  // 현재 캐시에 저장된 항목 수

// 캐시에서 URL을 검색하는 함수
int cache_find(char *url, char *response, int *response_size) {

  pthread_mutex_lock(&cache_mutex);
  
  for (int i = 0; i < cache_count; i++) {
      if (strcmp(cache[i].url, url) == 0) {  // URL이 캐시에 존재하면
          strcpy(response, cache[i].response);  // 캐시된 응답을 복사
          *response_size = cache[i].response_size;  // 응답 크기 설정

          pthread_mutex_unlock(&cache_mutex);
          
          return 1;  // 캐시 히트
      }
  }

  pthread_mutex_unlock(&cache_mutex);

  return 0;  // 캐시 미스
}

// 캐시에 새 데이터를 저장하는 함수
void cache_insert(char *url, char *response, int response_size) {
    if (response_size > MAX_OBJECT_SIZE) return;  // 너무 큰 객체는 캐시하지 않음

    pthread_mutex_lock(&cache_mutex);

    if (cache_count >= CACHE_SIZE) {
        // LRU 정책으로 가장 오래된 항목을 대체
        for (int i = 1; i < CACHE_SIZE; i++) {
            cache[i-1] = cache[i];  // 캐시 항목을 한 칸씩 앞으로 이동
        }
        cache_count--;
    }

    // 새로운 항목을 캐시에 저장
    strcpy(cache[cache_count].url, url);
    memcpy(cache[cache_count].response, response, response_size);
    cache[cache_count].response_size = response_size;
    cache_count++;

    pthread_mutex_unlock(&cache_mutex);
}
////chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-

int main(int argc, char **agrv)
{
  signal(SIGPIPE, SIG_IGN);
  int c_l_fd, *c_fd;
  char hname[MAXLINE], hport[MAXLINE];
  socklen_t clilen;
  struct sockaddr cliaddr;
  pthread_t t_id;

  printf("%s", user_agent_hdr);

  c_l_fd = Open_listenfd(agrv[1]);
  while (1)
  {

    clilen = sizeof(cliaddr);
    c_fd = (int*)Malloc(sizeof(int));
    *c_fd = Accept(c_l_fd, (SA *)&cliaddr, &clilen);
    Getnameinfo((SA *)&cliaddr, clilen, hname, MAXLINE, hport, MAXLINE, 0);
    printf("..connecting..\nhostname:%s\nconnect port: %s\n", hname, hport);
    // rio_writen(c_fd, "..connected..\n", 14);
    Pthread_create(&t_id, NULL, thread_test, c_fd);
  }
}

void *thread_test(void *vargp)
{
 int c_fd = *((int*)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  doit(c_fd);
  Close(c_fd);
  return NULL;
}

void doit(int c_fd)
{
  // signal(SIGPIPE, SIG_IGN);
  char buF[MAXLINE], meT[MAXLINE], urL[MAXLINE], veR[MAXLINE];
  char doM[MAXLINE], urI[MAXLINE], porT[MAXLINE], htT[MAXLINE];
  char head[MAXLINE], ptosF[MAXLINE], testbuF[MAXLINE];
  char ptosbuF[MAXLINE], stopbuF[MAXLINE], cache_data[MAX_OBJECT_SIZE];
  int s_fd, response_size;
  rio_t doitrio, serrio;


  
  Rio_readinitb(&doitrio, c_fd);
  Rio_readlineb(&doitrio, buF, MAXLINE);
  sscanf(buF, "%s %s %s", meT, urL, veR);
  printf("method: %s\nurl: %s\nversion: %s\n\n", meT, urL, veR);


////chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-
    // 캐시에서 데이터 검색
  if (cache_find(urL, cache_data, &response_size)) {
      printf("Cache hit: %s\n", urL);
      Rio_writen(c_fd, cache_data, response_size);  // 캐시 데이터 반환
      return;
  }
////chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-


  parser(urL, doM, urI, porT, htT);
  sprintf(ptosF, "%s %s %s\r\n", meT, urI, veR);
  sprintf(ptosF, "%sHost: %s\r\n", ptosF, doM);
  sprintf(ptosF, "%sProxy-Connection: close\r\n", ptosF);
  sprintf(ptosF, "%sUser-Agent: ProxyServer/1.0\r\n\r\n", ptosF);

  s_fd = Open_clientfd(doM, porT);

  Rio_readinitb(&serrio, s_fd);
  Rio_writen(s_fd, ptosF, strlen(ptosF));


////chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-
  int n;
  char server_response[MAX_OBJECT_SIZE];
  int total_size = 0;
  while ((n = Rio_readlineb(&serrio, stopbuF, MAXLINE)) > 0) {
      if (total_size + n < MAX_OBJECT_SIZE) {
          memcpy(server_response + total_size, stopbuF, n);
      }
      total_size += n;
      Rio_writen(c_fd, stopbuF, n);  // 클라이언트에 응답 전달
  }
  close(s_fd);

  // 캐시에 응답 저장
  if (total_size < MAX_OBJECT_SIZE) {
      cache_insert(urL, server_response, total_size);
  }
////chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-chatgpt-
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

