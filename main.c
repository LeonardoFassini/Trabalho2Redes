#include <pthread.h>
#include "leitura.c"
#include <unistd.h>

//VARIAVEIS
int porta[MAX], nh[MAX], distancia[MAX], vizinho[MAX];
int meuid;
pthread_t Enviador, Receptor, Tratar_snd, EnviarVetor, Tratar_cfg, Aguardar_mensagem, Enviar_mensagem;
pthread_mutex_t fila_cfg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fila_snd = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fila_msg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fila_enviar = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fila_snd_cfg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t vetor_distancia = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_alterou_vd = PTHREAD_MUTEX_INITIALIZER;
mensagem_t mensagem_cfg[MAX], mensagem_snd[MAX], mensagem_enviar[MAX], mensagem_snd_cfg[MAX];
int item_msg = 0, item_snd = 0, item_enviar = 0, item_cfg = 0, item_snd_cfg = 0, alterou_vd = 0, alterou = 0;

void die(char *s){
  perror(s);
  exit(1);
}

// Função que trata todas as mensagens de configuracao,
// tal que atualiza o vetor distancia se necessario,
// caso ele seja atualizado, envie o vetor distancia para seus vizinhos.
void *tratar_cfg(void*data){
  mensagem_t mensagem;
  struct sockaddr_in si_other;
  int s, slen = sizeof(si_other);
  int i;
  
  //CONFIGURAR THREAD
  if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) die("socket");
  memset((char*)&si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  if(inet_aton(SERVER, &si_other.sin_addr) == 0){
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }
  
  while(1){
    pthread_mutex_lock(&fila_snd);
    if(item_cfg){
      mensagem = mensagem_cfg[0];
      alterou = 1;
      item_cfg--;
      for(i = 0; i < item_cfg; i++) mensagem_cfg[i] = mensagem_cfg[i+1];
    }
    pthread_mutex_unlock(&fila_snd);
    // Se pegou uma mensagem, trata ela.
    if(alterou){
      alterou = 0;
      pthread_mutex_lock(&vetor_distancia);
      for(i = 0; i < MAX; i++)
	if(distancia[i] > mensagem.distancia[i] + distancia[mensagem.origem] ){
	  distancia[i] = mensagem.distancia[i] + distancia[mensagem.meio];
	  nh[i] = mensagem.meio;
	  alterou_vd = 1;
	}
      pthread_mutex_unlock(&vetor_distancia);
      if(alterou_vd){
	alterou_vd = 0;
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	printf("                 NOVO VETOR DISTANCIA\n");
	for(i = 0; i < MAX; i++) if(distancia[i] != INF) printf("%d -> %d: %d Custo: %d \n", meuid, i, nh[i], distancia[i]);
	strcpy(mensagem.status, "CFG");
	for(i = 0; i < MAX; i++)
	  mensagem.distancia[i] = distancia[i];
	mensagem.origem = meuid;
	mensagem.meio = meuid;
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	printf("          MENSAGENS DE CONFIGURACAO\n");
	for(i = 0; i < MAX; i++)
	  if(vizinho[i]){
	    // Selecionar a porta
	    mensagem.alvo = i;
	    si_other.sin_port = htons(porta[i]);
	    if(sendto(s, &mensagem, sizeof(mensagem), 0, (struct sockaddr *) &si_other, slen) == -1)
	      die("sendto()");
	    printf("Enviei uma mensagem para %d: %s\n", porta[i], mensagem.status);
	  }
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
      }
    }
  }
}

// Verifica se existe alguma mensagem na fila de mensagem.
// Em caso positivo, confira se o destino dessa mensagem sou eu.
// Se nao for eu o destino, envie essa mensagem para a fila de mensagens
// que ficara encarregada de enviar essa mensagem para o proximo salto
// de menor caminho em direcao ao ID alvo.
void *tratar_snd(void *data){
  mensagem_t mensagem;
  int i, alterou;
  while(1){
    alterou = 0;
    // Locka e pega a mensagem
    pthread_mutex_lock(&fila_snd);
    if(item_snd > 0){ // Tem alguma mensagem? se sim pega a mensagem e da shift a esquerda para as mensagem estarem no inicio.
      mensagem = mensagem_snd[0];
      alterou = 1;
      item_snd--;
      for(i = 0; i < item_snd; i++) mensagem_snd[i] = mensagem_snd[i+1];
    }
    pthread_mutex_unlock(&fila_snd);
    // Se pegou uma mensagem, trata ela.
    if(alterou){
      if(mensagem.alvo == meuid){ // Se o destino da mensagem sou eu.
	printf("----------------------------------------------\n\n");
	printf("Uma nova mensagem chegou!\n");
	printf("A mensagem veio de %d, sua origem é %d!\n", mensagem.meio, mensagem.origem);
	printf("O condeudo eh:\n '%s'\n", mensagem.mensagem);
	printf("----------------------------------------------\n");
      }
      else{ // Se nao sou eu, tenho que enviar essa mensagem para o proximo salto.
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
	printf("Uma mensagem chegou de %d, de origem %d com destino a %d\n\n", mensagem.meio, mensagem.origem, mensagem.alvo);
	mensagem.meio = meuid;
	mensagem.saltos++;
	// Tenta acessar a queue de mensagens e colocar a mensagem la.
	pthread_mutex_lock(&fila_enviar);
	mensagem_enviar[item_enviar++] = mensagem;
	pthread_mutex_unlock(&fila_enviar);
      }
    }
  }
}

// Se houver alguma mensagem na fila de mensagens, retire-a da fila
// e envie essa mensagem para quem deve ser enviado.
void *enviar_mensagem(void *data){
  mensagem_t mensagem;
  struct sockaddr_in si_other;
  int s, slen = sizeof(si_other);
  int alterou, i;
  //CONFIGURAR THREAD
  if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) die("socket");
  memset((char*)&si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  if(inet_aton(SERVER, &si_other.sin_addr) == 0){
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }
  
  
  while(1){
    alterou = 0;
    // Locka e tenta pegar uma mensagem da fila de envio
    pthread_mutex_lock(&fila_enviar);
    if(item_enviar){
      alterou = 1;
      mensagem = mensagem_enviar[0];
      item_enviar--;
      for(i = 0; i < item_enviar; i++) mensagem_enviar[i] = mensagem_enviar[i+1];
    }
    pthread_mutex_unlock(&fila_enviar);
    if(alterou){
      mensagem.meio = meuid;
      mensagem.saltos++;
      si_other.sin_port = htons(porta[nh[mensagem.alvo]]);
      if(sendto(s, &mensagem, sizeof(mensagem), 0, (struct sockaddr *) &si_other, slen) == -1)
	die("sendto()");
      printf("Repassei a mensagem para %d: %s\n", nh[mensagem.alvo], mensagem.mensagem);
    }
  }
}

// Aguarde receber uma mensagem do console, prepare ela
// e coloque na fila de mensagens a ser enviada.

void *aguardar_mensagem(void *data){
  mensagem_t mensagem;
  char string[101];
  int id_destino;
  int vizinho_alvo, novo_custo, opcao, i;
  while(1){
    printf("O que deseja?\n1- Mudar o custo do enlace.\n2- Enviar uma mensagem.\n");
    scanf("%d", &opcao);
    if(opcao == 1){
      printf("Digite para qual vizinho deseja mudar e qual o novo custo.\n");
      scanf("%d %d", &vizinho_alvo, &novo_custo);
      if(vizinho[vizinho_alvo] == 1){
	pthread_mutex_lock(&vetor_distancia);
	distancia[vizinho_alvo] = novo_custo;
	pthread_mutex_unlock(&vetor_distancia);
	alterou = 1;
	alterou_vd = 1;
      }
      else printf("esse no nao eh um vizinho\n");
    }
    else if(opcao == 2){
      printf("Digite a mensagem que deseja, aperte enter e digite o id destino\n");
      getchar();
      fgets(string, 100, stdin);
      scanf("%d", &id_destino);
      strcpy(mensagem.status, "SND");
      mensagem.origem = meuid;
      mensagem.meio = meuid;
      mensagem.alvo = id_destino;
      mensagem.saltos = 0;
      strcpy(mensagem.mensagem, string);
      pthread_mutex_lock(&fila_enviar);
      mensagem_enviar[item_enviar++] = mensagem;
      pthread_mutex_unlock(&fila_enviar);
    }
    else{
      printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
      printf("                 VETOR DISTANCIA\n");
      for(i = 0; i < MAX; i++) if(distancia[i] != INF) printf("%d -> %d: %d Custo: %d \n", meuid, i, nh[i], distancia[i]);
      printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    }
  }
}

// Prepare uma mensagem de configuracao com o meu vetor distancia
// e envie para meus vizinhos periodicamente.
void *enviar_vetor(void *data){
  struct sockaddr_in si_other;
  int s, slen = sizeof(si_other);
  int i;
  mensagem_t mensagem;

  
  //CONFIGURAR THREAD
  if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) die("socket");
  memset((char*)&si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  if(inet_aton(SERVER, &si_other.sin_addr) == 0){
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }

  // Inicializar
  while(1){
    // Preparar a mensagem de configuracao;
    strcpy(mensagem.status, "CFG");
    for(i = 0; i < MAX; i++)
      mensagem.distancia[i] = distancia[i];
    // Enviar
    mensagem.origem = meuid;
    mensagem.meio = meuid;
    for(i = 1; i < MAX; i++){
      if(vizinho[i]){
	// Selecionar a porta
	mensagem.alvo = i;
	si_other.sin_port = htons(porta[i]);
	if(sendto(s, &mensagem, sizeof(mensagem), 0, (struct sockaddr *) &si_other, slen) == -1)
	  die("sendto()");
	//printf("Enviei uma mensagem para %d, tendo como origem: %d e meio %d\n", porta[i], mensagem.origem, mensagem.meio);
      }
    }
    // Durma até chegar a vez de enviar novamente a configuracao.
    sleep(10);
  }
}

// Fique no aguardo de alguma mensagem, ela pode ser de
// configuracao ou de envio, caso seja de configuracao, envie para a fila que trata
// as configuracoes, alterando ou nao seu vetor distancia.
void *receptor(void *data){
  struct sockaddr_in si_me, si_other;
  int s, slen = sizeof(si_other), recv_len;
  mensagem_t mensagem;
  
  if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) die("socket");
  memset((char*) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(porta[meuid]);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(s, (struct sockaddr*) &si_me, sizeof(si_me)) == -1)
    die("bind");
  
  // Comece a ouvir e a tratar as mensagens
  while(1){
    fflush(stdout);
    // Quando eu receber uma mensagem
    if((recv_len = recvfrom(s, &mensagem, sizeof(mensagem), 0, (struct sockaddr *) &si_other, &slen)) == -1)
      die("recvfrom()");
    // Verificar que mensagem é.
    // Mensagem de Configuracao.
    if(mensagem.status[0] == 'C'){
      pthread_mutex_lock(&fila_cfg);
      // Colocar a mensagem na queue de configuracao.
      mensagem_cfg[item_cfg++] = mensagem;
      pthread_mutex_unlock(&fila_cfg);
    }
    // Mensagem de Envio.
    else if(mensagem.status[0] == 'S'){
      pthread_mutex_lock(&fila_msg);
      // Colocar na fila de mensagens de envio.
      mensagem_snd[item_snd++] = mensagem;
      pthread_mutex_unlock(&fila_msg);
    }
  }
}

// Leia os enlaces.config e roteador.config, retirando e salvando os dados que me interessam.
// Crie as threads que irao tratar de todo o roteamento.
int main(int argc, char*argv[]){
  int cont = 0, i, id, linha, coluna, custo;
  char string[MAX], a[MAX], ip[MAX][MAX];
  FILE *entrada;
  
  for(i = 0; i < MAX; i++){ nh[i] = -1; distancia[i] = INF; }
  meuid = atoi(argv[1]);
  printf("Meu id é: %d\n", meuid);
  if(!(entrada = fopen("roteador.config", "r")))
    printf("Erro ao abrir o arquivo 'roteador.config'\n");
  //Leia os ids, ips e sockets dos roteadores.
  while(fgets(string, 100, entrada) != NULL){ 
    cont++;
    leiaConfig(string, a, &id, ip, porta);
  }
  
  // Agora imprima eles.
  printf("ID | IP | PORTA\n");
  for(i = 1; i <= cont; i++)
    printf("%d %s %d\n", i, ip[i], porta[i]);
  printf("\n");
  
  entrada = fopen("enlaces.config", "r");
  printf("ID | ID ALVO | CUSTO\n");
  while(fgets(string, 100, entrada) != NULL){
    leiaEnlace(string, a, &linha, &coluna, &custo);
    if(linha == meuid){
      vizinho[coluna] = 1;
      distancia[coluna] = custo;
      nh[coluna] = coluna;
      printf("%d %d %d\n", linha, coluna, custo);
    }
  }
  for(i = 0; i < cont; i++){ printf("%d -> %d: %d\n", meuid, i, nh[i]); }
  printf("\n\n Vetor Distancia original:\n");
  for(i = 0; i < MAX; i++) if(distancia[i] != INF) printf("%d -> %d: %d Custo: %d \n", meuid, i, nh[i], distancia[i]);
  printf("\n");
  pthread_create(&EnviarVetor, NULL, enviar_vetor, NULL);
  pthread_create(&Receptor, NULL, receptor, NULL);
  pthread_create(&Tratar_snd, NULL, tratar_snd, NULL);
  pthread_create(&Tratar_cfg, NULL, tratar_cfg, NULL);
  pthread_create(&Enviar_mensagem, NULL, enviar_mensagem, NULL);
  pthread_create(&Aguardar_mensagem, NULL, aguardar_mensagem, NULL);
  pthread_join(EnviarVetor, NULL);
  pthread_join(Receptor, NULL);
  pthread_join(Tratar_snd, NULL);
  pthread_join(Tratar_cfg, NULL);
  pthread_join(Enviar_mensagem, NULL);
  pthread_join(Aguardar_mensagem, NULL);
  return 0;
}
