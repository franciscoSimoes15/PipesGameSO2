#pragma once

#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <Windows.h>
#include <time.h>
#define H_SEM_LOCK 0
#define H_SEM_ESCRITA 1
#define H_SEM_LEITURA 2
#define EVENT_PARAAGUA _T("EVENT_PARAAGUA")
#define EVENT_PAUSE _T("EVENT_PAUSE")
#define HEVENT_ESCREVE 0
#define HEVENT_PARAAGUA 1
#define HEVENT_PAUSE 2
#define HEVENT_ESCREVE_CLIENTE 3
#define MAXBUFFER 256
#define MAX_MAP_SIZE 20
#define SERVER_KEY_NAME _T("SERVERKEY")
#define SHAREDMEM _T("SHAREDMEMORY")
#define MUTEX_NAME _T("MUTEXMULTIACCESS")
#define SEM_WRITE _T("SEMAPHOREWRITE")
#define SEM_READ _T("SEMAPHOREREAD")
#define EVENT_SHUTDOWN _T("EVENTSHUTDOWN")
#define EVENT_TEMP _T("EVENTEMP")
#define EVENT_WRITE _T("EVENTWRITE")
#define EVENT_WRITE_CLIENT _T("EVENT_WRITE_CLIENT")
#define SEM_MONITOR _T("SEMMONITOR")
#define SEM_SERVER_LOCK _T("SEMAPHORE_SERVER_LOCK") // prevenir somente uma instância
#define MAX_SERVER_USERS 1
#define WATER_ATT_NAME _T("TIMERAGUA")
#define MAP_ATT_NAME _T("MAPASIZE")
#define MEMBUFFSIZE sizeof(SharedMemory)
#define THREAD_EVENT _T("THREADEVENT") 
#define PRODCONS_EVENT_THREAD _T("PRODCONSEVENTTHREAD")
#define MUTEX_DATA_THREAD _T("MUTEXDATATHREAD")
#define PIPE_SERVER TEXT("\\\\.\\pipe\\teste")
#define N 2

enum {
	ERROR_CREATING_KEY,
	OPENED_EXISTING_KEY,
	CREATED_NEW_KEY,
	NO_EXISTING_KEY,
	EXISTING_KEY,
	NO_EXISTING_ATT,
	ERROR_SET_ATT_VALUE,
	MAP_SIZE_EXCEEDED,
	EXISTING_ATT,
	NON_EXISTING_ATT,
	WATER_INVALID_VALUE,
	EXISTING_SERVER
};


typedef struct {
	int atual[2]; // ponto x,y atual
	int prox[2]; // ponto x,y proximo
	int direcao; //0 esquerda, 1 cima, 2 direita, 3 baixo
}Posicao;

typedef struct {
	// ponto origem
	DWORD origemX;
	DWORD origemY;
	// ponto Destino
	DWORD destinoX;
	DWORD destinoY;

	DWORD mapSize; // mapSize dada uma instância
	DWORD waterTime;// tempo de começo de fluxo de água

	BOOL modoAleatorio; // se on ou off
	TCHAR arrMapa[MAX_MAP_SIZE][MAX_MAP_SIZE]; // mapa em si

	Posicao p; // posicao water
}Mapa;



//Só inicia Monitor quando o filemap tiver criado no servidor


//Monitor é que envia os comandos (Exclusivamente monitor)
typedef struct {
	int x, y;
	TCHAR comando[MAXBUFFER];
}BufferCircular;

typedef struct {
	HANDLE hPipe; // handle do pipe
	OVERLAPPED overlap;
	BOOL activo; //representa se a instancia do named pipe esta ou nao ativa, se ja tem um cliente ou nao
} PipeData;
typedef struct {
	Mapa mapa;
	TCHAR nome[100];
	DWORD modo;
	DWORD xPos, yPos;
	DWORD estadoJogo;
	DWORD pontos;
	DWORD nivel;
	DWORD tempo;
}Client;
//representa a nossa memoria partilhada
typedef struct {
	int nMonitores; //nProdutores
	int posR; //proxima posicao de leitura
	int posW; //proxima posicao de escrita
	BufferCircular buffer[2]; //buffer circular (array de estruturas)

	BOOL correAgua; // se tem água a correr ou não
	BOOL modoPecaAleatorio; // se as peças surgem de modo aleatório ou não
	Mapa mapa; // servidor->monitor

}SharedMemory;
typedef struct {
	HANDLE hEventos[2];
	PipeData hPipes[2];
	HANDLE hMutexData;
	int terminar;
	Client dadosCliente;

}ThreadData; // editar
typedef struct {
	HANDLE hMapFile; // handle mapeamento
	SharedMemory* sharedMem; //ponteiro para a memoria partilhada
	HANDLE hMutex;
	int terminar; // 1 para sair, 0 em caso contrário (trinco)
	HANDLE hSem[3];
	HANDLE hEvent[4];
	ThreadData td;
	HANDLE hThread[4];
}DadosSync;




// SERVIDOR
DWORD createAttribute(TCHAR* attName, DWORD value, HKEY hkey);
DWORD createKey(TCHAR* keyName, TCHAR* path, HKEY* hkey);
BOOL initMemAndSync(DadosSync* cdata, DWORD mapSize, DWORD waterTime);
void geraOrigem(Mapa* mapa);
DWORD random(DWORD mapSize);
void mostraMapa(Mapa m);
void fechaTorneira(int segundos);
int executaComando(BufferCircular cel, SharedMemory* mem);
void updateMapa(SharedMemory* mem);
DWORD WINAPI ThreadCicloAgua(LPVOID param);
void menuComandosServidor();
BOOL leComando(DadosSync* dados);
DWORD WINAPI ServidorListener(LPVOID param);
DWORD WINAPI ThreadOuveCliente(LPVOID param);
DWORD WINAPI ThreadConsumidor(LPVOID param);
TCHAR escolheTubo(BOOL value, TCHAR* pecas);
BOOL jogarPeca(int x, int y, Mapa* mapa, TCHAR* pecas);
void carregaMapa(Mapa* mapa);
DWORD verificaPosicao(Mapa m, Posicao p);
DWORD verificaTubo(Mapa m, Posicao* p);
// MONITOR
BOOL initMemAndSync(DadosSync* cdata);
void menuComandosMonitor();
void mostraMapa(Mapa m);
TCHAR* verificaComando();
DWORD WINAPI ThreadMonitorLeitura(LPVOID dados);
DWORD WINAPI ThreadEncerraMonitor(LPVOID param);
DWORD WINAPI ThreadProdutor(LPVOID param);

// CLIENTE
DWORD WINAPI OuveServidor(LPVOID lparam);
void AddWindows(HWND hWnd);
int verificaX(int xPos);
int verificaY(int yPos);