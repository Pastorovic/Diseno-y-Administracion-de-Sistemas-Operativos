//este archivo es el fichero fuente que al compilarse produce el ejecutable Ej1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/times.h>

#define MAX 256 		//tamaño maximo del mensaje

//imprime la tarea realizada por un proceso
void printTask(int process, char *task)
{
	printf("El proceso P%d (PID=%d, Ej1) %s\n", process, getpid(), task);
}

//imprime el error causado y termina el programa
void printError(char *error)
{
	perror(error);
	exit(-1);
}

int main()
{
	char mensaje[MAX];	//mensaje recibido por la entrada estandar
	char msgPipe[MAX];	//mensaje recibido por la tuberia sin nombre
	int pids[2];		//almacena los pids de P2 y P3
	int tuberia[2];		//descriptores de la tuberia sin nombre
	int f1;			//descriptor de fichero1
	int msqid;		//identificador de la cola de mensajes
	int CLK_TCK;		//tics por segundo
	int fd, h;		//para el calculo de estadisticas
	float tt,tu,tn,tcu,tcn;
	key_t llave;		//llave para la cola de mensajes
	clock_t t[2];		//contabilizan los ticks al inicio y al final
	struct tms pb[2];	//tiempos de ejecucion al inicio y al final
	struct
	{
		long tipo;
		pid_t pid;
	} msgQueue;		//almacena el mensaje y el tipo de una cola de mensajes
	msgQueue.tipo=2;	//se asigna el tipo 2

	//almacena los tiempos de ejecucion hasta el momento
	t[0]=times(&pb[0]);

	//se abre este mismo fichero (fuente1.c)
	//repetidas veces para poder tomar tiempos para las estadisticas acordes
	printTask(1, "esta anotando estadisticas de uso de CPU...");
	for(h=1;h<=5000000;h++) 
	{
		fd=open("fuente1.c",O_RDONLY);
		close(fd); 
	}
	
	//se crea la tuberia
	if(pipe(tuberia)==-1) printError("error al crear la tuberia");

	//se crea P2
	printTask(1, "crea el proceso hijo P2");
	if((pids[0]=fork())==-1) printError("error en la creación de P2");
	//se ejecuta P2
	else if(pids[0]==0)
	{
		//se lee el mensaje de la tuberia sin nombre
		if(read(tuberia[0], msgPipe, MAX)==-1)
			printError("error en la lectura de la tuberia sin nombre");
		printTask(2, "lee el mensaje de la tuberia sin nombre");
		//se cierra el descriptor de escritura de la tuberia
		if(close(tuberia[1])==-1) printError("error al cerrar el descriptor de escritura");

		//se crea el fichero FIFO fichero1
		if(mknod("fichero1", S_IFIFO | 0666, 0)==-1) printError("error en la creacion de fichero1");
		//se abre el fichero1 con permisos de lectura/escritura
		if((f1=open("fichero1", O_RDWR))==-1) printError("error al abrir fichero1");
		//se escribe el mensaje en fichero1
		if(write(f1, msgPipe, strlen(msgPipe)+1)==-1) printError("errir al escribir en fichero1");
		printTask(2, "escribe el mensaje en fichero1");

		//se ejecuta Ej2
		printTask(2, "ejecuta Ej2");
		execv("./Trabajo2/Ej2", NULL);
	}
	//se ejecuta P1
	else
	{
		//se lee el mensaje desde la entrada estandar
		printf("\nIntroduzca el mensaje: ");
		gets(mensaje);
		printf("\n");
		//se escribe el mensaje en la tuberia
		if(write(tuberia[1], mensaje, strlen(mensaje)+1)==-1)
			printError("error en la escritura de la tuberia sin nombre");
		printTask(1, "escribe el mensaje en la tuberia sin nombre");
		//se cierra el descriptor de escritura de la tuberia
	
		if(close(tuberia[1])==-1) printError("error al cerrar el descriptor de lectura");

		//se crea la llave a la que se asociara la cola de mensajes
		if((llave=ftok("./Trabajo2/Ej1", 'D'))==-1) printError("error al crear la llave");

		//se crea la cola de mensajes
		if((msqid=msgget(llave, IPC_CREAT | 0600))==-1) printError("error al crear la cola de mensajes");
		//se lee el mensaje de la cola de mensajes		
		if((msgrcv(msqid, &msgQueue, MAX, 2, 0))==-1) printError("error al leer de la cola de mensajes");
		printTask(1, "lee el pid de P3 de la cola de mensajes");
		//se obtiene el pid de P3 del mensaje leido
		pids[1]=msgQueue.pid;
		//se borra la cola de mensajes
		msgctl(msqid, IPC_RMID, 0);
		
		//se mata a P2 y P3
		if((kill((pids[0]), SIGKILL))==-1) printError("error al matar a P2");
		printTask(1, "borra el proceso P2");
		if((kill(pids[1], SIGKILL))==-1) printError("error al matar a P3");
		printTask(1, "borra el proceso P3");
		//se borra fichero1
		if((unlink("fichero1"))==-1) printError("error al borrar fichero1");
		printTask(1, "borra el fichero FIFO fichero1");
		//se espera a que P2 haya terminado
		wait();	

		//se obtienen los ticks por segundo del sistema
		CLK_TCK=sysconf(_SC_CLK_TCK);
		//almacena los tiempos de ejecucion hasta el momento
		t[1]=times(&pb[1]);		
		//se imprimen las estadisticas
		tt=(float)(t[1]-t[0])/CLK_TCK;
		tu=(float)(pb[1].tms_utime-pb[0].tms_utime)/CLK_TCK;
		tn=(float)(pb[1].tms_stime-pb[0].tms_stime)/CLK_TCK;
		tcu=(float)(pb[1].tms_cutime-pb[0].tms_cutime)/CLK_TCK;
		tcn=(float)(pb[1].tms_cstime-pb[0].tms_cstime)/CLK_TCK;
		printf("\n-----------------------------------------------------------------------------\n");
		printf("MOSTRANDO ESTADISTICAS");
		printf("\n-----------------------------------------------------------------------------\n");
		printf("Tiempo real                                                   = %4.2g segundos\n", tt);
		printf("Tiempo de uso de la CPU en modo usuario                       = %4.2g segundos\n", tu);
		printf("Tiempo de uso de la CPU en modo nucleo                        = %4.2g segundos\n", tn);
		printf("Tiempo de uso de la CPU de los procesos hijos en modo usuario = %4.2g segundos\n", tcu);
		printf("Tiempo de uso de la CPU de los procesos hijos en modo nucleo  = %4.2g segundos\n", tcn);
		printf("Porcentaje de uso de la CPU                                   = %4.1f%% \n",
		       ((tu+tn+tcu+tcn)/tt)*100);
		printf("-----------------------------------------------------------------------------\n");
	}
	return 0;
}
