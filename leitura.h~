#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <string.h>

#define MAX 112
#define INF 112345
#define BUFLEN 101
#define SERVER "127.0.0.1"

// ESTRUTURAS
typedef struct mensagem{
  char status[3];
  int alvo;
  int origem;
  int meio;
  int saltos;
  char mensagem[100];
  int distancia[MAX];
} mensagem_t;

void leiaConfig(char s[], char a[], int *id, char ip[][MAX], int porta[MAX]);
