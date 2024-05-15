#include<stdio.h>
#include <arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include <locale.h>


#define TamAux 25

struct Mensagem{
    char* msg;
    unsigned long *tam_msg;
};

void Cliente(char *endereco, char *porta);
unsigned Menu(struct Mensagem *buffer);
unsigned QuantCaracter(char caracter, char *string);
unsigned CasoEspecialS(char *string);
//void

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
	if(argc > 3 || argc<1)
	{
        perror("Argumentos inválidos ou erro!");
	}

    Cliente(argv[1], argv[2]);


	return 0;
}


//Função Cliente

void Cliente(char *endereco, char *porta)
{
	int sock;
	struct sockaddr_in sAddr;
	struct Mensagem buffer;
    char aux[TamAux];
    char *resposta;
    unsigned long tamResposta;
    int n, cont=0;

	//setando variavel sAddr
	memset((void *) &sAddr, 0, sizeof(struct sockaddr_in));
	sAddr.sin_family = AF_INET;
	sAddr.sin_addr.s_addr = INADDR_ANY;
	sAddr.sin_port = 0;

    //criação de socket e linkagem ao endereço do servidor


	//conectar ao servidor que foi passado por parametros
	sAddr.sin_addr.s_addr = inet_addr(endereco);
	sAddr.sin_port = htons(atoi(porta));


    while (1){
        if(Menu(&buffer)==0){
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            bind(sock, (const struct sockaddr *) &sAddr, sizeof(sAddr));
            if(connect (sock, (const struct sockaddr *) &sAddr, sizeof(sAddr)) != 0)
            {
                perror("cliente");
                return;
            }

            send(sock, buffer.tam_msg, sizeof(unsigned long), 0);
            recv(sock, aux, 25, 0);//aviso para avisar que memória foi alocada no servidor
            aux[16] = '\0';
            send(sock, buffer.msg, *buffer.tam_msg-1, 0);

            recv(sock, &tamResposta, sizeof(unsigned long ), 0);
            resposta = malloc(tamResposta+1);
            send(sock, aux, strlen(aux)+1, 0);//avisar que a memória foia alocada

            cont=0;
            while (1){
                n = recv(sock, aux, sizeof(aux), 0);
                memcpy(resposta+cont, aux, n);
                cont+=n;
                if(n!=TamAux) break;
            }
            resposta[tamResposta] = '\0';
            printf("%s\n", resposta);
            free(resposta);
            free(buffer.msg);
            free(buffer.tam_msg);
            close(sock);
        }
    }

	//enviar alguma coisa ao servidor.
	/*snprintf(buffer, 128, "dados do cliente");
	sleep(1);
	printf("mandou %i caracteres\n", send(sock, buffer, strlen(buffer), 0));
	sleep(1);
	printf(" recebeu %i caracteres\n", recv(sock, buffer, 25, 0));

	sleep(1);
	close(sock);*/
}

unsigned Menu(struct Mensagem *buffer){
    char *mensagem = NULL;
    size_t nleitura=0;
    ssize_t tamMsg;
    unsigned long *aux = malloc(sizeof(unsigned long ));

    tamMsg = getline(&mensagem, &nleitura, stdin);//lê uma linha de caracteres do arquivo de entreda e aloca automaticamente
    mensagem[tamMsg-1] = '\0';//a função pega o caracter '\n'
    *aux = tamMsg;

    switch (mensagem[0]) {
        case 'N'://registro
            if(QuantCaracter(' ', mensagem)!=2)
                return 1;
            break;
        case 'S':
            if(CasoEspecialS(mensagem)!=0)
                return 1;
            break;
        case 'R':
            if(QuantCaracter(' ', mensagem)!=2)
                return 1;
            break;
        case 'L':
            if(QuantCaracter(' ', mensagem)!=1)
                return 1;
            break;

        default:
            printf("");
    }



    buffer->msg = mensagem;
    buffer->tam_msg = aux;

    return 0;
}

unsigned CasoEspecialS(char *string){
    int i, j, k;
    unsigned cont=0;
    unsigned long tamString = strlen(string);
    char *QuartoParametro;
    unsigned long tamQuartoParametro;

    for(i=0; i< tamString+1; i++){
        if(string[i]==' '){
            cont++;
            if(cont==3){
                if(string[i+1]!='[')
                    return 1;
                else
                    break;
            }
            if(string[i] == '\0')
                return 1;
        }
    }
    for(j = tamString; j>i+1; j--){
        if(string[j] == ' '){
            if(string[j-1]==']') {
                QuartoParametro = string + j + 1;

                if((strcmp(QuartoParametro, "Inicio")==0) || (strcmp(QuartoParametro, "Fim")==0))
                    return 0; //parametro para colocar paragrafo no inicio fim
                else{
                    if((QuartoParametro[0]!='P') || (QuartoParametro[1]!='_'))
                        return 1; //quarto parametro inválido
                    tamQuartoParametro = strlen(QuartoParametro);
                    for(k=2; k< tamQuartoParametro; k++){
                        if((QuartoParametro[k]<'0') || (QuartoParametro[k]>'9'))
                            return 1;//valor numérico inválido
                    }
                }
                return 0;//parâmetros estão corretos
            }else {
                return 1;//parâmetro inválido
            }
        }
    }
    return 1;
}


unsigned QuantCaracter(char caracter, char *string){
    int i;
    unsigned cont=0;
    for(i=0; i< strlen(string); i++){
        if(string[i]==caracter){
            cont++;
        }
    }
    return cont;
}