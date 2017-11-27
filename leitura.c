#include "leitura.h"

void leiaConfig(char s[], char a[], int *id, char ip[][MAX], int porta[MAX]){
  int j = 0, i; 

  for(i = 0; s[i] != ' '; i++) a[j++] = s[i];
  a[j] = '\0';
  *id = atoi(a);
  while(s[i] == ' ') i++;
  j = 0;
  for(; s[i] != ' '; i++) a[j++] = s[i];
  a[j] = '\0';
  porta[*id] = atoi(a);
  j = 0;
  while(s[i] == ' ') i++;
  for(; s[i] != '\n'; i++) ip[*id][j++] = s[i];
  ip[*id][j] = '\0';
}

void leiaEnlace(char string[], char a[], int *linha, int *coluna, int *custo){
  int j = 0, i;
  for(i = 0; string[i] != ' '; i++) a[j++] = string[i];
  a[j] = '\0';
  *linha = atoi(a);
  while(string[i] == ' ') i++;
  j = 0;
  for(; string[i] != ' '; i++) a[j++] = string[i];
  a[j] = '\0';
  *coluna = atoi(a);
  while(string[i] == ' ') i++;
  j = 0;
  for(; string[i] != '\n'; i++) a[j++] = string[i];
  a[j] = '\0';
  *custo = atoi(a);
}

// recema uma matriz de adjacencia, o source, quantos vértices esse grafo tem, e a quantidade da matriz de adjacencia.
/*
void bford(int eg[][MAX], int s, int v, int nh){
  int dist[v], pai[v];
  for(i = 0; i < v; i++){ dist[i] = INF; pai[v] = NULL;}
  dist[s] = 0;
  for(i = 0; i < v-1; i++)
    for(u = 0; u < v; u++)
      for(j = 0; j < MAX; j++){
	v = vizinho[u][j];
	if(v != -1 && dist[v] > dist[u] + v){
	  dist[v] = dist[u] + eg[u][v];
	  pai[v] = u;
	}
      }
  for(i = 0; i <= v; i++)
    printf("%d", pai[v]);
}

*/
