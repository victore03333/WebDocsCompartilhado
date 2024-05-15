//Universidade Estadual de Mato Grosso do Sul
//Curso de Ciência da Computação
//Disciplina de Redes de Computadores

//Este servidor utiliza a função fork() para gerenciar múltiplos clientes.
//Ao se fazer a chamada de sistema fork(), cria-se uma duplicata exata do programa e, um novo processo filho é iniciado para essa cópia.



//Inclusão das bibliotecas
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <netinet/in.h>
#include <string.h>


//A biblioteca <sys/wait.h> e a biblioteca <signal.h> são necessárias para evitar a criação de zombies.
//Zombies são processos filhos que aparecem quando processos pais deixam de existir sem ser feita a chamada wait() ou waitpid() nos processos filhos.
#include <sys/wait.h>
#include <signal.h>
#include <locale.h>

#define TamBuffer 25

int semaforo_id;

void protocoloDeRede(char *protocolo, int socket);
unsigned protocoloN(char *protocolo);
    unsigned Registrar(char *arg1, char *arg2);
int protocoloS(char *protocolo);
    int Envio(char *arg1, char *arg2, char *arg3, char *arg4);
        int paragrafoExiste(int numParagrafo, char *arquivo);
char *protocoloR(char *protocolo);
    char *recebimento(char *arg1, char *arg2);
        void realocar(unsigned long tamAdicional, char **mensagem);
char* protocoloL(char *protocolo);
    char *Lista(char *arg1);
unsigned usuarioExiste(char *nomeUsuario);

void sigchld_handler(int signo)  //manipulador de sinais. Ele simplesmente faz a chamada waitpid para todo filho que for desconetado.
{
  while (waitpid(-1, NULL, WNOHANG) > 0);  //A ideia de se chamar em um laço é que não se tem certeza que há uma correlação um-para-um entre os filhos desconectados e as chamadas ao manipualador de sinais.
}  //valore lembrar que o posix não permite a criação de filas nas chamadas de sinal. Ou seja, pode acontecer de chamar o manipulador após vários filhos já terem sido desconectados.


//Declaração de variáveis tradicionais
int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    char *teste = "P_235";

    struct sockaddr_in sAddr;
    int meu_socket;
    int novo_socket;
    char aviso[25] = "Memória Alocada";
    int resultado;
    int leitor;
    int pid;
    int valor;
    FILE *arq;
    int cont = 0;

    if(argc!=2){
        perror("Parametro invalido");
        return 1;
    }

    if(atoi(argv[1])<0){
        perror("Parametro invalido");
        return 1;
    }

    arq = fopen("ArquivosUsuarios", "a");//cria arquivos de usuários
    if(arq==NULL){
        perror("erro ao criar arquivo de usuários\n");
    }
    fclose(arq);
    arq = fopen("NomesArquivos", "a");//cria arquivos de usuários
    if(arq==NULL){
        perror("erro ao criar arquivo de usuários\n");
    }
    fclose(arq);


    key_t semaforo_key = ftok("sem_compartilhado", 'S');


    // Cria o semáforo
    semaforo_id = semget(semaforo_key, 1, IPC_CREAT | 0666);
    if (semaforo_id == -1) {
        perror("Erro ao criar o semáforo");
        exit(1);
    }

    // Inicializa o semáforo
    if (semctl(semaforo_id, 0, SETVAL, 1) == -1) {
        perror("Erro ao inicializar o semáforo");
        exit(1);
    }
    
    meu_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//criação do socket que aceitará as chamadas de conexão
    if(meu_socket < 0){
        fprintf(stderr, "falha ao criar socket");
        return 0;
    }

    valor = 1;
    //SO_REUSEADDR significa que as regras utilizadas para a validação de endereço feita pelo bind permite a reutilização de endereços locais.
    resultado = setsockopt(meu_socket, SOL_SOCKET, SO_REUSEADDR, &valor, sizeof(valor));
    if (resultado < 0) {
        perror("Erro");
        return 0;
    }
    
    //Uso do bind para associar a porta com todos endereços
    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons(atoi(argv[1]));
    sAddr.sin_addr.s_addr = INADDR_ANY;

    resultado = bind(meu_socket, (struct sockaddr *) &sAddr, sizeof(sAddr));
    if (resultado < 0) {
        perror("Erro no bind");
        return 0;
    }

    
    //colocando o socket para ouvir a chegada de conexões
    resultado = listen(meu_socket, 5);
    if (resultado < 0) {
        perror("Erro no listen");
        return 0;
    }

    signal(SIGCHLD, sigchld_handler); //Ativando o manipulador de sinais antes de entrar no laço.


    while (1) {

        novo_socket = accept(meu_socket, NULL ,NULL);  //Antes da chamada ser aceita (retornada), chama-se o fork() para a criação de novos processos.
        if ((pid = fork()) == 0) {  //Se retornar zero é porque estamos no processo inicial, caso contrário retornamos o PID do novo processo filho.

            unsigned long *tam_mensagem = malloc(sizeof(unsigned long));
            char bufferRecv[TamBuffer];
            char *novaMensagem = NULL;
            int n = 0;

            printf("Processo filho %i criado.\n", getpid());
            close(meu_socket);



            //Uma vez com o processo filho, fecha-se o processo listen. Lembre-se que todos os são copiados do processo pai para o filho.
            recv(novo_socket, tam_mensagem, sizeof(unsigned long),0); //le alguma coisa do cliente e devolve para o mesmo.
            novaMensagem = malloc(*tam_mensagem* sizeof(char));
            printf("%lu\n", *tam_mensagem);

            send(novo_socket, aviso, strlen(aviso) + 1, 0);

            while (1) {
                n = recv(novo_socket, bufferRecv, sizeof(bufferRecv), 0);
                fprintf(stderr, "CLIENTE: %s | n: %d\n", bufferRecv, n);
                memcpy(novaMensagem+cont, bufferRecv, n);
                cont+=n;
                if (n != TamBuffer) break;
            }
            novaMensagem[*tam_mensagem - 1] = '\0';
            fprintf(stderr, "CLIENTE: %s | n: %d\n", novaMensagem, n);

            protocoloDeRede(novaMensagem, novo_socket);



            if (novaMensagem != NULL) {
                free(novaMensagem);
            }
            if(tam_mensagem!=NULL){
                free(tam_mensagem);
            }


            close(novo_socket);  //Esta linha só é alcançada no processo pai. Uma vez que o processo filho tem uma cópia do socket cliente. O processo pai a sua referência e decrementa o contador no kernel.
            printf("Processo filho %i terminado.\n", getpid());
            exit(0);
        }
        close(novo_socket);
    }
}

void protocoloDeRede(char *protocolo, int socket){
    char *resposta = NULL;
    int nExplicacao;
    unsigned long tamResposta;
    char aviso[25];
    printf("%s\n", protocolo);

    switch (protocolo[0]) {
        case 'N':
            resposta = malloc(4* sizeof(char));
            memcpy(resposta, "N ", 2);
            nExplicacao = protocoloN(protocolo);
            switch (nExplicacao) {
                case 0:
                    strcat(resposta, "0");
                    break;
                case 1:
                    strcat(resposta, "1");
                    break;
                case 2:
                    strcat(resposta, "2");
                    break;
            }
            break;
        case 'S':
            resposta = malloc(5* sizeof(char));
            memcpy(resposta, "S ", 2);
            nExplicacao = protocoloS(protocolo);
            printf("%i\n", nExplicacao);
            switch (nExplicacao) {
                case 0:
                    strcat(resposta, "0");
                    break;
                case -1:
                    strcat(resposta, "-1");
                    break;
                case -2:
                    strcat(resposta, "-2");
                    break;
                case -3:
                    strcat(resposta, "-3");
                    break;
                default:
                    printf("Erro\n");
            }
            break;
        case 'R':
            resposta=protocoloR(protocolo);
            break;
        case 'L':
            resposta=protocoloL(protocolo);
            break;
        default:
            printf("Erro\n");
    }
    printf("Resposta Final: %s", resposta);
    tamResposta = strlen(resposta);
    send(socket, &tamResposta, sizeof(unsigned long ),0);
    recv(socket, aviso, 25, 0);//esperando resposta do clinte para sua alocação de memoria
    send(socket, resposta, tamResposta, 0);


    if (resposta!=NULL)
        free(resposta);
}

unsigned protocoloN(char *protocolo){
    int saida;
    char *arg1;
    char *arg2;
    int i;
    unsigned long tamProt = strlen(protocolo);
    char *pch;
    struct sembuf sem_op;

    pch = memchr(protocolo+2, ' ', tamProt);
    i = pch-protocolo+1;
    arg1 = malloc((i-3+1)* sizeof(char));
    memcpy(arg1, protocolo+2, i-3);
    arg2 = malloc((tamProt-i)*sizeof(char));
    memcpy(arg2,protocolo+i, tamProt-i);



    sem_op.sem_num = 0;
    sem_op.sem_op = -1;  // Aguarda pelo semáforo
    sem_op.sem_flg = 0;
    semop(semaforo_id, &sem_op, 1);

    //sessão crítica
    saida = Registrar(arg1, arg2);
    //fim sessão crítica

    sem_op.sem_num = 0;
    sem_op.sem_op = 1;  // Libera o semáforo
    sem_op.sem_flg = 0;
    semop(semaforo_id, &sem_op, 1);

    free(arg1);
    free(arg2);

    return saida;


}

unsigned Registrar(char *arg1, char *arg2){

    FILE *arq;
    char quebraDeLinha = '\n';

    if(usuarioExiste(arg1)==0){//verifica se usuario ja existe
        printf("retorno 1\n");
        return 1;
    }

    if(access(arg2, F_OK)==0){//verifica se arquivo ja existe
        printf("retorno 2\n");
        return 2;
    }

    arq = fopen(arg2, "a");//criação arquivo
    fwrite(arg1, sizeof(char), strlen(arg1), arq);
    fwrite(&quebraDeLinha, sizeof(char), 1, arq);
    fclose(arq);

    arq = fopen("ArquivosUsuarios", "a");
    fwrite(arg1, sizeof(char), strlen(arg1), arq);
    fwrite(&quebraDeLinha, sizeof(char), 1, arq);
    fclose(arq);

    arq = fopen("NomesArquivos", "a");
    fwrite(arg2, sizeof(char), strlen(arg2), arq);
    fwrite(&quebraDeLinha, sizeof(char), 1, arq);
    fclose(arq);

    return 0;//arquivo criado com sucessso
}



int protocoloS(char *protocolo){
    int referencia_byte, aux, i;
    char *arg1;
    char *arg2;
    char *arg3;
    char *arg4;
    char *pch;
    int saida;
    struct sembuf sem_op;

    pch = memchr(protocolo+2, ' ', strlen(protocolo));
    referencia_byte = pch - protocolo + 1;
    arg1 = malloc((referencia_byte - 3 + 1)* sizeof(char));
    memcpy(arg1, protocolo+2, referencia_byte - 3);

    aux = referencia_byte;
    pch = memchr(protocolo + referencia_byte + 1, ' ', strlen(protocolo));
    referencia_byte = pch - protocolo + 1;
    arg2 = malloc((referencia_byte - aux)* sizeof(char));
    memcpy(arg2, protocolo+aux, referencia_byte - aux - 1);

    referencia_byte = pch - protocolo + 2;//+2 pra desconsiderar a " ["
    for(i= strlen(protocolo); i>referencia_byte; i--){
        if(protocolo[i]==' '){
            arg3 = malloc((i-referencia_byte-1)* sizeof(char));//-2 do espaço e colchetes
            memcpy(arg3, protocolo+referencia_byte, i-referencia_byte-1);//pra desconsiderar a "] "
            break;
        }
    }
    arg4 = malloc((strlen(protocolo)-i+1)* sizeof(char));
    memcpy(arg4, protocolo+i+1, strlen(protocolo)-i+1);


    sem_op.sem_num = 0;
    sem_op.sem_op = -1;  // Aguarda pelo semáforo
    sem_op.sem_flg = 0;
    semop(semaforo_id, &sem_op, 1);

    saida = Envio(arg1, arg2, arg3, arg4);

    sem_op.sem_num = 0;
    sem_op.sem_op = 1;  // Libera o semáforo
    sem_op.sem_flg = 0;
    semop(semaforo_id, &sem_op, 1);

    printf("Envio: %d\n", saida);
    return saida;
}

int Envio(char *arg1, char *arg2, char *arg3, char *arg4){
    printf("%s %s %s %s\n", arg1, arg2, arg3, arg4);
    FILE *arq;
    char *texto=NULL, *novoTexto=NULL, *pch, *aux;
    unsigned tamArq, tamArg1 = strlen(arg1), tamArg3 = strlen(arg3);
    int numParagrafo, cont=0;
    char quebraLinha = '\n', abreC='[', fechaC = ']';

    if(usuarioExiste(arg1)!=0){//verifica se usuario não existe
        printf("retorno 1\n");
        return -1;
    }

    if(strcmp(arg2, "ArquivosUsuarios")==0 || strcmp(arg2, "NomesArquivos")==0){//diretórios  que o cliente não pode acessar
        return -1;
    }

    if(access(arg2, F_OK)!=0){//verifica se arquivo não existe
        printf("retorno 2\n");
        return -2;
    }

    if(!(arg4[0]=='I' || arg4[0]=='F')){
        numParagrafo = atoi(arg4+2);
        if(paragrafoExiste(numParagrafo, arg2)!=0){
            return -3;
        }
    }

    arq = fopen(arg2, "r");

    fseek(arq, 0, SEEK_END);
    tamArq = (unsigned)ftell(arq);
    fseek(arq, 0, SEEK_SET);

    texto = malloc(tamArq * sizeof(char) + 1);
    fread(texto, sizeof(char), tamArq, arq);
    texto[tamArq] = '\0';

    fclose(arq);

    aux = texto;
    switch (arg4[0]) {
        case 'F'://colocar texto no final do arquivo
            arq=fopen(arg2, "a");
            fwrite(&abreC, sizeof(char), 1, arq);
            fwrite(arg1, sizeof(char), strlen(arg1), arq);
            fwrite(&fechaC, sizeof(char), 1, arq);
            fwrite(arg3, sizeof(char), strlen(arg3), arq);
            fwrite(&quebraLinha, sizeof(char), 1, arq);
            break;
        case 'I':
            arq = fopen(arg2, "w");
            novoTexto = (char*)malloc((tamArq+ tamArg1+tamArg3)* sizeof(char)+4);
            pch = memchr(texto, '\n', strlen(texto));
            memcpy(novoTexto, texto, pch-texto+1);
            novoTexto[pch-texto+1]='\0';
            strcat(novoTexto, "[");
            strcat(novoTexto, arg1);
            strcat(novoTexto, "]");
            strcat(novoTexto, arg3);
            strcat(novoTexto, "\n");
            strcat(novoTexto, pch+1);
            fwrite(novoTexto, sizeof(char), strlen(novoTexto), arq);
            break;
        case 'P':
            arq = fopen(arg2, "w");
            novoTexto = (char*)malloc((tamArq+ tamArg1+tamArg3)* sizeof(char)+4);
            aux=memchr(aux+1, '\n', strlen(aux));//ignorar dono do arquivo
            while (cont<numParagrafo){
                aux=memchr(aux+1, '\n', strlen(aux));
                cont++;
            }
            memcpy(novoTexto, texto, aux-texto+1);
            novoTexto[aux-texto+1]='\0';
            strcat(novoTexto, "[");
            strcat(novoTexto, arg1);
            strcat(novoTexto, "]");
            strcat(novoTexto, arg3);
            strcat(novoTexto, "\n");
            strcat(novoTexto, aux+1);
            fwrite(novoTexto, sizeof(char), strlen(novoTexto), arq);
            break;
        default:
            printf("erro\n");
    }
    fclose(arq);

    if(texto!=NULL)
        free(texto);
    if(novoTexto!=NULL)
        free(novoTexto);

    return 0;
}

int paragrafoExiste(int numParagrafo, char *arquivo){
    FILE *file = fopen(arquivo, "r");
    char *listaUsuario, *aux;
    long int tamArquivo;
    unsigned paragrafoArquivo;
    int cont=0;


    fseek(file, 0, SEEK_END);
    tamArquivo = ftell(file);
    fseek(file, 0, SEEK_SET);

    listaUsuario = malloc(tamArquivo* sizeof(char)+1);
    fread(listaUsuario, sizeof(char), tamArquivo, file);
    listaUsuario[tamArquivo+1] = '\0';


    aux = listaUsuario;//verifica as ocorrencias de \n, este q determina a quantidade de paragrafos no arquivo
    aux = memchr(aux+1, '\n', strlen(aux));//ignorar o paragrafo que indica o dono do arquivo
    if(aux==NULL){

        return 1;
    }
    while ((aux = memchr(aux+1, '\n', strlen(aux)))!=NULL)
        cont++;

    free(listaUsuario);
    if(numParagrafo<cont-1)//numParagrafo 0 indica o primeiro paragrafo, somamos mais um para determinar se a quantidade de paragrafos  que o cliente esta pedindo é <= a real quantidade do paragrafo
        return 0;
    else
        return 1;

}

char *protocoloR(char *protocolo){
    char *arg1;
    char *arg2;
    int i;
    unsigned long tamProt = strlen(protocolo);
    char *pch;
    char* resposta;

    pch = memchr(protocolo+2, ' ', tamProt);
    i = pch-protocolo+1;
    arg1 = malloc((i-3+1)* sizeof(char));
    memcpy(arg1, protocolo+2, i-3);
    arg2 = malloc((tamProt-i)* sizeof(char));
    memcpy(arg2,protocolo+i, tamProt-i);

    printf("aqui1\n");
    resposta=recebimento(arg1, arg2);
    return resposta;
}
char* recebimento(char *arg1, char *arg2){
    char *texto, *aux, *aux2,  *donoParagrado;
    unsigned tamArq, tamDono, tamParagrafo;
    FILE *arq;
    unsigned cont=4;
    char terminador = '\0';
    char *resposta = malloc(5);
    strcpy(resposta, "R 0\n");
    char *retorno12;

    if(usuarioExiste(arg1)!=0){//verifica se usuario não existe
        retorno12 = malloc(5);
        strcpy(retorno12, "R -1");
        printf("retorno 1\n");
        return retorno12;
    }

    if(access(arg2, F_OK)!=0){//verifica se arquivo não existe
        retorno12 = malloc(5);
        strcpy(retorno12, "R -1");
        printf("retorno 2\n");
        return retorno12;
    }

    arq = fopen(arg2, "r");

    fseek(arq, 0, SEEK_END);
    tamArq = (unsigned)ftell(arq);
    fseek(arq, 0, SEEK_SET);

    texto = malloc(tamArq * sizeof(char) + 2);
    fread(texto, sizeof(char), tamArq, arq);
    texto[tamArq] = '\0';
    strcat(texto, "\n");

    printf("aqui2 %s", texto);
    aux=memchr(texto, '\n', strlen(texto));
    aux+=1;
    printf("aqui3\n");

    if(aux[0]=='\n'){
        fclose(arq);
        printf("aqui4\n");
        retorno12 = malloc(5);
        strcpy(retorno12, "R -3");
        printf("retorno 3\n");
        return retorno12;//não tem paragrafos
    }
    aux = texto;
    while(1){
        aux = memchr(aux+1, '\n', strlen(texto));
        aux2 = memchr(aux, ']', strlen(aux));
        if(aux2==NULL)
            break;
        tamDono = aux2-aux-2;
        donoParagrado = malloc( tamDono* sizeof(char)+1);
        memcpy(donoParagrado, aux+2, tamDono);
        if(strcmp(arg1, donoParagrado) == 0){
            aux2 = memchr(aux2, '\n', strlen(aux2));
            tamParagrafo = aux2-aux+1;
            realocar(tamParagrafo+1+1, &resposta);//
            memcpy(resposta+cont, aux, tamParagrafo);
            cont+=tamParagrafo;
            memcpy(resposta+cont, &terminador, 1);

        }
        free(donoParagrado);
    }
    if(cont==4){
        fclose(arq);
        retorno12 = malloc(5);
        strcpy(retorno12, "R -3");
        printf("retorno 3\n");
        return retorno12;
    }
    fclose(arq);
    return resposta;
}

void realocar(unsigned long tamAdicional, char **mensagem){
    unsigned long tamMensagem = strlen(*mensagem);
    char* aux = malloc(tamMensagem+1);
    memcpy(aux, *mensagem, tamMensagem+1);
    free(*mensagem);
    *mensagem = malloc(tamMensagem+tamAdicional+1);
    memcpy(*mensagem, aux, tamMensagem+tamAdicional);

}

char* protocoloL(char *protocolo){
    char *arg1 = malloc((strlen(protocolo)-2)* sizeof(char));//-2 desconsiderar os dois primeiros caracteres
    char *resposta;
    memcpy(arg1, protocolo+2, strlen(protocolo)-2);

    resposta = Lista(arg1);
    return resposta;
}


char *Lista(char *arg1){
    char *retorno1, *aux, *leArquivos, *resposta,*pch, *donoArq;
    FILE *arq;
    unsigned long tamNoomesArq, contLinhasNomeArq=0, numparagrafos=0, tamDonoArq=0;
    char**arquivos;
    int i=0;
    char strNumParagrafos[11];//o tipo unsigned long tem no máximo 10 valores

    resposta = malloc(5);
    memcpy(resposta, "L 0\n", 5);

    if(usuarioExiste(arg1)!=0){//verifica se usuario não existe
        retorno1 = malloc(5);
        strcpy(retorno1, "L -1");
        printf("retorno 1\n");
        return retorno1;
    }

    arq = fopen("NomesArquivos", "r");

    fseek(arq, 0, SEEK_END);
    tamNoomesArq = (unsigned)ftell(arq);
    fseek(arq, 0, SEEK_SET);

    leArquivos = malloc(tamNoomesArq * sizeof(char) + 1);
    fread(leArquivos, sizeof(char), tamNoomesArq, arq);
    leArquivos[tamNoomesArq] = '\0';

    fclose(arq);

    if(tamNoomesArq==0){
        free(leArquivos);
        return resposta;
    }


    aux=leArquivos;
    while ((aux= memchr(aux+1, '\n', strlen(aux)))!=NULL){
        contLinhasNomeArq++;
    }

    for(i=0; i < contLinhasNomeArq; i++){
        arquivos = (char**)malloc(contLinhasNomeArq * sizeof(char*));
    }

    aux = leArquivos;
    i=0;
    printf ("Splitting string \"%s\" into tokens:\n", leArquivos);
    pch = strtok (leArquivos, "\n");
    while (pch != NULL){
        arquivos[i] = malloc(strlen(pch)+1);
        memcpy(arquivos[i], pch, strlen(pch)+1);
        i++;

        pch = strtok (NULL, "\n");
    }
    free(leArquivos);

    i=0;
    while (i < contLinhasNomeArq){
        arq=fopen(arquivos[i], "r");
        fseek(arq, 0, SEEK_END);
        tamNoomesArq = (unsigned)ftell(arq);
        fseek(arq, 0, SEEK_SET);

        leArquivos = malloc(tamNoomesArq * sizeof(char) + 1);
        fread(leArquivos, sizeof(char), tamNoomesArq, arq);
        leArquivos[tamNoomesArq] = '\0';

        //passo1
        realocar(strlen(arquivos[i]), &resposta);
        strcat(resposta, arquivos[i]);

        pch = strtok (leArquivos, "\n");
        donoArq = malloc(strlen(leArquivos)+1);
        aux = memchr(leArquivos, '\0', sizeof(leArquivos));
        tamDonoArq = aux-leArquivos+1;
        memcpy(donoArq, leArquivos, tamDonoArq);
        numparagrafos=0;
        while (pch != NULL){
            numparagrafos++;
            pch = strtok (NULL, "\n");
        }
        //passo2
        snprintf(strNumParagrafos, sizeof(strNumParagrafos), "%lu", numparagrafos-1);//caso tenha algum paragrafo

        realocar(strlen(strNumParagrafos)+1, &resposta);
        strcat(resposta, " ");
        strcat(resposta, strNumParagrafos);
        //passo3
        realocar(strlen(donoArq)+2, &resposta);
        strcat(resposta, " ");
        strcat(resposta, donoArq);
        strcat(resposta, "\n");

        free(leArquivos);
        free(donoArq);
        fclose(arq);
        i++;
    }

    for(i=0; i < contLinhasNomeArq; i++){
        free(arquivos[i]);
    }
    free(arquivos);

    return resposta;
}

unsigned usuarioExiste(char *nomeUsuario){
    FILE *file = fopen("ArquivosUsuarios", "r");
    char *listaUsuario, *aux, *pch;
    long int tamArquivo;


    fseek(file, 0, SEEK_END);
    tamArquivo = ftell(file);
    fseek(file, 0, SEEK_SET);

    listaUsuario = malloc(tamArquivo * sizeof(char) + 1);
    fread(listaUsuario, sizeof(char), tamArquivo, file);
    listaUsuario[tamArquivo] = '\0';

    fclose(file);

    printf("\n%s %s\n", listaUsuario, nomeUsuario);

    if(tamArquivo==0)
        return 1;//arquivo vazio

    pch = strtok (listaUsuario, "\n");
    while (pch != NULL){
        if(strcmp(pch, nomeUsuario)==0){
            free(listaUsuario);
            return 0;
        }

        pch = strtok (NULL, "\n");
    }

    return 1;
}
