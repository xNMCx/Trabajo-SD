// main.c
//SON TRES PAQUETES TCP EN LOS PEDIDOS PARA HACER UN HANDSHACKING
//
//Descrición: crear un cliente web (HTML) llamado mi_wget...
//
//Fecha: 01/03/18
//Nombre: Julio Carmona Gonzalez
//Compilacion: gcc main.c -o mi_wclient -D DEBUG
//
//Ejemplo de uso: ./mi_wget 193.145.322.5 /index.html
//

//Seccion de cabeceras

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>



//Mis herramientas de depuracion

#ifdef DEBUG
#define log(fmt, args...)			fprintf(stderr, fmt "\n",## args)
#else
#define log(ftm, args...)
#endif

void pexit(char *msg){
	perror(msg);
	exit(EXIT_FAILURE);
}

//Seccion de definiciones

#define HTTP_REQUEST_PATTERN  "%s %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: keep-Alive\r\n\r\n"

char *peticion;

//El verdadero cliente web (HTTP)

void clienteweb(char *dirIP, int puerto, char *recurso, char *peticion) 
{
	int sockfd;
	static struct sockaddr_in serv_addr;
	char HTTP_request[200];
	char HTTP_response[500];
	int bLeidos;

	//SOCKET: obtener un canal por internet

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		pexit("socket() fallo");
	}

	//CONNECT: conectar con un host (direccion IP) y una aplicacion (host)

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(puerto);
	serv_addr.sin_addr.s_addr = inet_addr(dirIP);

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		pexit("connect() c mamo");
	}

	//WRITE: nos comunicamos con el servidor web (HTTP)

	sprintf(HTTP_request, HTTP_REQUEST_PATTERN, proceso, recurso, dirIP, "mi_wget");
	log(" Enviando %d bytes: \n\n%s\n", (int)strlen(HTTP_request), HTTP_request);
	write(sockfd, HTTP_request, strlen(HTTP_request));

	//READ: recogemos lo que nos envie el servidor (HTTP)

	while((bLeidos = read(sockfd, HTTP_response, sizeof(HTTP_response))) > 0)
		write(STDOUT_FILENO, HTTP_response, bLeidos);

	//CLOSE: cierro el canal

	close(sockfd);

}

int mostrarUso(char *aplicacion)
{
	fprintf(stderr, "Debe proporcionar una URI valida y una peticion (GET, DELETE, PUT, POST o HEAD).\n");
	fprintf(stderr, "Uso: %s URI peticion\n", aplicacion);
	fprintf(stderr, "Ejemplo: %s http://www.dtic.ua.es:80/~dmarcos/sd/index.html GET (o PUT, DELETE...) 2>> DEFAULT.log > index.html\n", aplicacion);


//Punto de entrada de mi aplicacion

int main(int argc, char *argv[])
{
	char *protocol;
	char *host;
	int port;
	char *resource;
	char *host_ip;
	char *cadena[5];
	int cont = 1;
	char *str;

	struct servent *sp;								//Para convertir Protocolo en Puerto
	struct hostent *host_struct;					//Para convertir DNS a IP
	// Verificamos los argumentos de entrada
	if (argc != 3){
		mostrarUso(argv[0]);
		return EXIT_FAILURE;
	}

	peticion = argv[2];
	str = argv[1];

	//Primera rotura de cadena (fuera del while)
	char *token = strtok(str, ":");
	//Realizamos un while despues del inicial a str para obtener desglosada la URI
	cadena[0] = token;
	while (token != NULL)
	{
		token = strtok(NULL, ":");
		if(cont < 4){
	        cadena[cont] = token;
		}
		cont = cont + 1;
		token = strtok(NULL, "/");
		if(cont < 4){
	        cadena[cont] = token;
		}
		cont = cont + 1;
    
	}
	cadena[5] = '\0';
	token = strtok(cadena[1], "://");
	cadena[1] = token;

	//Extraemos la informacion desde los parametros de entrada
	protocol = cadena[0];
	host = cadena[1];
	port = atoi(cadena[2]);
	resource = cadena[3];

	//Obtenemos el PROTOCOLO eb sy puerto si no hay puerto [puerto 0]
	if ((protocol[0]!= '\0' ) && (port==0))
	{
		sp = getservbyname(protocol, "tcp");
		if(!sp)
		{
			fprintf(stderr, "No se ha podido encontrar un puerto valido para el protocolo %s\n", protocol);
			return EXIT_FAILURE;
		}
		port = ntohs(sp->s_port);
	}

	//Convertimos el DNS (host) en su equivalente IP (host_ip)

	host_struct = gethostbyname(host);
	if(host_struct != NULL)
		host_ip = inet_ntoa(* (struct in_addr *) host_struct->h_addr);

	// Mostrar todas las variables relevantes
	log(" * protocolo = <%s>", protocol);
	log(" * host = <%s>", host);
	log(" * port = <%d>", port);
	log(" * ruta = <%s>", resource);
	log(" * IP = %s", host_ip);

	//Comprobamos que lo tenemos todo
	if(!port || !host_ip || resource[0]=='\0'){
		mostrarUso(argv[0]);
		return EXIT_FAILURE;
	}

	//Invocar al verdadero cliente web (HTTP)
	clienteweb(host_ip, port, resource, peticion);


	return EXIT_SUCCESS;


} 
