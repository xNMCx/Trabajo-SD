//  Proyecto Servidor HTTP para Sistemas Distribuidos
//	Compilación: gcc main.c -o mi_httpd

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>


#define SERWEB_VERSION_NAME                 "1.0"

//conjunto de variables para lectura de http.conf
#define FILENAME                            "httpd.conf"
#define MAXBUF                              1024
#define DELIM                               " "

#define HTTP_RESPONSE_PATTERN                    "HTTP/1.1 %s\r\nContent-Lenght: 206\r\nConnection: Close\r\nContent-Type: text/html\r\n\r\n<html>\n<head>\n<title>%s</title>\n</head>\n<body>\n<h1>%s</h1>\nThe requested URL, file type of operation is not allowed on this simple static file webserver.\n</body>\n</html>\n"

#define MAX_REQUEST_SIZE                    4048
#define MAX_PATH_SIZE                       256
#define BUFSIZE                             8096

#ifdef DEBUG
#define log(fmt, args...)          fprintf(stderr, fmt "\n",## args)
#else
#define log(ftm, args...)
#endif

//conjunto de variable locales definidas en el main
int port, maxClients;
char documentRoot, directoryIndex;

struct config
{
   char documentRoot[MAXBUF];
   char directoryIndex[MAXBUF];
   char maxClients[MAXBUF];
   char port[MAXBUF];
}

struct config get_config(char *filename)
{
        struct config configstruct;
        FILE *file = fopen (filename, "r");

        if (file != NULL)
        {
                char line[MAXBUF];
                int i = 0;

                while(fgets(line, sizeof(line), file) != NULL)
                {
                        char *cfline;
                        cfline = strstr((char *)line,DELIM);
                        cfline = cfline + strlen(DELIM);

                        if (i == 0){
                            memcpy(configstruct.documentRoot,cfline,strlen(cfline));
                        }
                        else if (i == 1){
                            memcpy(configstruct.directoryIndex,cfline,strlen(cfline));
                        }
                        else if (i == 2){
                            memcpy(configstruct.maxClients,cfline,strlen(cfline));
                        }
                        else if (i == 3){
                            memcpy(configstruct.port,cfline,strlen(cfline));
                        }

                        i++;
                } // End while
                fclose(file);
        } // End if file



        return configstruct;

}


int AtenderProtocoloHTTP(int sd)
{
    char buffer[MAX_REQUEST_SIZE] = "\0";
    long bLeidos = 0;
    int  result;
    char *copia;
    char *valores[4];
    int cont = 0;
    char HTTP_response[500];

    log("atendiendo la solicitud HTTP...");

    // HTTP REQUEST: leemos la cabecera (debe ser siempre menor que MAX_REQUEST_SIZE)
    bLeidos = read(sd, buffer, sizeof(buffer)-1);
    if (bLeidos < 0)
    {
        fprintf(stderr, "ERROR: [%s] leyendo del socket\n", strerror(errno));
        return -1;
    }
    buffer[bLeidos] = '\0';
    log("Recibidos %ld bytes\n%s", bLeidos, buffer);

    //comprobacion de primera linea

    copia = strtok(buffer, "\r\n");

    char *token = strtok(copia, " ");

    while (token != NULL)
    {
        valores[cont] = token;
        token = strtok(NULL, ":");
        cont = cont+1;
    }

    valores[4] = "\0";

    if(valores[0] == null || valores[1] == null || valores[2] == null){
        sprintf(HTTP_response, "400 bad request", "400 bad request", "bad request" );
        result = write(sd, HTTP_response strlen(HTTP_response));
        return -1;
    }

    if(strcmp(valores[0], "HTML/1.1") != 0){
        sprintf(HTTP_response, "505 HTML version not Supported");
        result = write(sd, HTTP_response strlen(HTTP_response));
        return -1;
    }

    if(strcmp(valores[0], "GET") != 0 && strcmp(valores[0], "PUT") != 0 && strcmp(valores[0], "DELETE") != 0 && strcmp(valores[0], "POST") != 0 && strcmp(valores[0], "HEAD") != 0){
        sprintf(HTTP_response, HTTP_RESPONSE_PATTERN, "405 method not allowed", "405 method not allowed", "method not allowed");
        result = write(sd, HTTP_response strlen(HTTP_response));
        return -1;
    }

    //realizacion de cada una de las peticiones



    // HTTP RESPONSE: respondemos con un mensaje
    log("Enviando request (%ld bytes): \n%s", strlen(HTTP_BAD_REQUEST), HTTP_BAD_REQUEST);
    result = write(sd, HTTP_BAD_REQUEST, strlen(HTTP_BAD_REQUEST));

    return result;
}   /* de AtenderProtocoloHTTP */


int ObtenerSocket(int puerto)
{
    int                 sfdServidor = -1;   // socket para esperar peticiones
    struct sockaddr_in  addr;
    int                 error;

    // SOCKET: Abrimos un socket donde esperar peticiones de servicio
    log("SOCKET: Abriendo un socket...");
    sfdServidor = socket(AF_INET, SOCK_STREAM, 0);
    if(sfdServidor == -1)
    {
        fprintf(stderr, "ERROR: [%s] abriendo el socket\n", strerror(errno));
        return -1;
    }

    // BIND: Asociamos el puerto en el que deseamos esperar con el socket obtenido
    log("BIND: asociando el socket con el puerto de escucha...");
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons((u_short)puerto);

    error = bind(sfdServidor, (struct sockaddr *)&addr, sizeof(addr));
    if(error > 0)
    {
        fprintf(stderr, "ERROR: [%s] en bind\n", strerror(errno));
        close(sfdServidor);
        return -1;
    }

    // LISTEN: Nos ponemos a la escucha en el socket, indicando cuántas atendemos a la vez
    log("LISTEN: indicamos que atienda haste %d conexiones simultaneas", maxClients);
    error = listen(sfdServidor, maxClients);
    if(error < 0)
    {
        fprintf(stderr, "ERROR: [%s] en listen\n", strerror(errno));
        close(sfdServidor);
        return -1;
    }

    return sfdServidor;
}   // de ObtenerSocket


int ConectarConCliente(int sfdServidor)
{
    int                 sfdCliente = -1;    // socket del hijo. Atiende cada nueva petición
    struct sockaddr_in  cliente;
           socklen_t    tamCliente = sizeof (cliente);
    struct hostent      *host;

    // ACCEPT: Esperamos hasta que llegue una comunicación, para la que nos dan un nuevo socket
    log("ACCEPT: esperando nueva conexion...");
    sfdCliente = accept(sfdServidor, (struct sockaddr *)&cliente, &tamCliente);

    log("Comprobando conexion entrante... (resolucion inversa del dns)");
    host = gethostbyaddr((char *)&cliente.sin_addr, sizeof(cliente.sin_addr), cliente.sin_family);
    log("Conexion aceptada a [%s] %s: %u", host->h_name, inet_ntoa(cliente.sin_addr), cliente.sin_port);

    return sfdCliente;
}   // de ConectarConCliente


void LanzarServicio(int puerto)
{
    int sfdServidor = -1;   // socket del padre. Escucha nuevas peticiones
    int sfdCliente = -1;    // socket del hijo. Atiende cada nueva petición
    int result;

    sfdServidor = ObtenerSocket(port);
    if(sfdServidor == -1)
    {
        fprintf(stderr, "ERROR: no se ha podido establecer la conexion de red\n");
        exit(-1);
    }


    while (1) {
        sfdCliente = ConectarConCliente(sfdServidor);
        if(sfdCliente == -1)
        {
            fprintf(stderr, "ERROR: no se ha podido conectar con el cliente\n");
            exit(-1);
        }

        switch (fork()) {
            case -1:    // Error
                fprintf(stderr, "ERROR: [%s] ejecutando fork\n", strerror(errno));
                close(sfdCliente);
                close(sfdServidor);
                exit (-1);

            case  0:    // El hijo atiende la nueva solicitud
                log("PROCESO HIJO %d: atendiendo al protocolo de aplicacion en socket %d", (int)getpid(), sfdCliente);
                close(sfdServidor);
                result = AtenderProtocoloHTTP(sfdCliente);      // Atendemos el PROTOCOLO concreto
                log("PROCESO HIJO %d: terminando proceso", (int)getpid());
                close(sfdCliente);
                exit (result);

            default:    // El padre se queda a la espera de nuevas conexiones
                log("PROCESO PADRE %d: buscando una nueva conexion", (int)getpid());
                close(sfdCliente);
        }
    }   /* de while infinito */
}

int main(int argc, char *argv[])
{
    if(argc == 1){}
        //estructura del fichero httpd.conf


        struct config configstruct;

        configstruct = get_config(FILENAME);

        /* Puerto como entero */
        port = atoi(configstruct.port);
        maxClients = atoi(configstruct.maxClients);
        directoryIndex = configstruct.directoryIndex;
        documentRoot = configstruct.documentRoot;
    }

    else if(argc == 5){

        documentRoot = argv[1];
        directoryIndex = argv[2];
        maxClients = atoi(argv[3]);
        port = atoi(argv[4]);


    }

    //Mostrando informacion de configuracion
    log("CONFIG: Puerto de escucha %d", port);
    log("CONFIG: Max Clientes: %d", maxClients);
    log("CONFIG: Recuerso por defecto: %s", directoryIndex);
    log("CONFIG: Directorio Raiz: %s", documentRoot);

    //Lanzamos el sevicio que atiende al protocolo (protocolo estandar)
    log("Lanzamos servicio en puerto %d", port);
    LanzarServicio(port);

    //En realidad nunca llegaremois aqui
    return 0;

}	/* de main */
