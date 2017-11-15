//este archivo es el fichero fuente que al compilarse produce el ejecutable Ej1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>
//#include <sys/msg.h>

#define MAX 256 		//tamaño maximo del mensaje

//imprime la tarea realizada por un proceso
void printTask(int process, char *task)
{
	printf("El proceso P%d (PID=%d, Ej2) %s\n", process, getpid(), task);
}

//imprime el error causado y termina el programa
void printError(char *error)
{
	perror(error);
	exit(-1);
}

int main()
{
	char msgFIFO[MAX];	//mensaje de fichero1
	char *vc1;		//mensaje a almacenar en memoria compartida
	int pid3;		//proceso P3	
	int f1;			//descriptor del fichero1
	int shmid;		//identificador de la zona de memoria compartida asociado a llave
	int sem1;		//identificador del semaforo
	int fd, h;		//para el calculo de estadisticas
	key_t llave;		//llave que se asociara a la region de memoria compartida y al semaforo

	//se abre el fichero FIFO fichero1
	if((f1=open("fichero1", O_RDWR))==-1) printError("error al abrir fichero1");
	//se lee en mensaje del fichero FIFO
	if(read(f1, msgFIFO, MAX)==-1) printError("error al leer el mensaje de fichero1");
	printTask(2, "lee el mensaje que contiene el fichero FIFO fichero1");
	//se cierra el fichero FIFO fichero1
	if(close(f1)==-1) printError("error al cerrar fichero1");

	//se crea la llave a la que se asociara la MC y al semaforo
	if((llave=ftok("fichero1", 'P'))==-1) printError("error al crear la llave");

	//se crea la zona de memoria compartida
	if((shmid=shmget(llave, MAX*sizeof(char), IPC_CREAT | 0600))==-1)
		printError("error al crear la zona de memoria compartida");
	//se une la zona de memoria compartida al espacio de direcciones virtuales de vc1
	if((vc1=shmat(shmid, 0, 0))==(char *)-1)
		printError("error al asignar el espacio de direcciones virtuales a la zona de memoria compartida");
	
	//se crea el semaforo
	if((sem1=semget(llave, 1, IPC_CREAT | 0600))==-1) printError("error en la creacion del semaforo");
	//se inicia el semaforo a 0
	if(semctl(sem1, 0, SETVAL, 0)==-1) printError("error al iniciar el semaforo");
	//se crea la operacion V()
	struct sembuf opV[1];
	opV[0].sem_num=0;
	opV[0].sem_op=1; 
	opV[0].sem_flg=0;

	//se crea P3
	printTask(2, "crea el proceso hijo P3");
	if((pid3=fork())==-1) printError("error en la creación de P2");
	//se ejecuta P3
	else if(pid3==0)
	{
		//se ejecuta Ej3
		printTask(3, "ejecuta Ej3");
		execv("./Trabajo2/Ej3", NULL);
	}
	//se ejecuta P2
	else
	{
		//se abre el fichero fuente1.c que se encuentra en el directorio actual
		//repetidas veces para poder tomar tiempos para las estadisticas acordes
		printTask(2, "esta anotando estadisticas de uso de CPU...");
		for(h=1;h<=5000000;h++) 
		{
			fd=open("fuente1.c",O_RDONLY);
			close(fd); 
		}
		//se duerme a P2 durante 1 segundo
		sleep(1);

		//se escribe el mensaje en memoria compartida
		strcpy(vc1, msgFIFO);
		printTask(2, "escribe el mensaje en la variable compartida vc1");

		//se ejecuta la operacion V() del semaforo
		if(semop(sem1, opV, 1)==-1) printError("error al realizar operacion V del semaforo");
		printTask(2, "levanta el semaforo y se suspende su ejecucion");

		//se espera a que P3 haya terminado y se suspende la ejecucion de P2
		wait();
		pause();
	}
}
