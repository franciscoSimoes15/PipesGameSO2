
#include "..//Servidor//comunicacao.h"

BOOL initMemAndSync(DadosSync* cdata) {
	cdata->hSem[H_SEM_LOCK] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, SEM_SERVER_LOCK);

	if (cdata->hSem[H_SEM_LOCK] == NULL) {
		_tprintf(_T("ERROR OPENING SEM LOCK (%d)\nINICIE UMA INSTÂNCIA SERVIDOR...\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		return FALSE;
	}
	cdata->hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHAREDMEM);
	if (cdata->hMapFile == NULL) {
		_tprintf(_T("ERROR OPENING FILE MAP...\nInicialize o servidor...(Tem 5 segundos...)\n"));
		for (int i = 5; i >= 0; i--) {
			_tprintf(_T("Tempo restante:%d\n"), i);
			Sleep(1000);
		}
		cdata->hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHAREDMEM);
		if (cdata->hMapFile == NULL)
			return FALSE;
	}
	cdata->sharedMem = (SharedMemory*)MapViewOfFile(cdata->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (cdata->sharedMem == NULL) {
		_tprintf(TEXT("Erro no MapViewOfFile\n"));
		return FALSE;
	}
	cdata->hMutex = OpenMutex(NULL, FALSE, MUTEX_NAME);
	
	cdata->hSem[H_SEM_ESCRITA] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, SEM_WRITE);
	if (cdata->hSem[H_SEM_ESCRITA] == NULL) {
		_tprintf(_T("ERROR OPENING SEMAPHORE WRITE (%d)\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		return FALSE;
	}

	cdata->hSem[H_SEM_LEITURA] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, SEM_READ);
	
	if (cdata->hSem[H_SEM_LEITURA] == NULL) {
		_tprintf(_T("ERROR OPENING SEMAPHORE WRITE (%d)\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		return FALSE;
	}
	
	cdata->hEvent[HEVENT_ESCREVE] = OpenEvent(EVENT_ALL_ACCESS, TRUE, EVENT_WRITE);

	if (cdata->hEvent[HEVENT_ESCREVE] == NULL) {

		_tprintf(_T("ERROR OPENING EVENT (%d)\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		CloseHandle(cdata->hMutex);
		CloseHandle(cdata->hSem[H_SEM_ESCRITA]);
		CloseHandle(cdata->hSem[H_SEM_LEITURA]);

		return FALSE;
	}
	
	cdata->terminar = 0;
	return TRUE;
}
void menuComandosMonitor() {
	_tprintf(_T("Comandos Disponíveis monitor:\n"));
	_tprintf(_T("\tBloco - Inserir blocos Intransponíveis\n"));
	_tprintf(_T("\tParaagua x - Fecha torneira por x segundos\n"));
	_tprintf(_T("\tModoAleatorio y - Sequência de peças aleatórias (y - representa on/off)\n"));
}
void mostraMapa(Mapa m) {
	_tprintf(_T("\n\n"));
	_tprintf(_T("\t┌"));
	for (DWORD i = 0; i < m.mapSize; i++)_tprintf(_T("─"));
	_tprintf(_T("┐\n"));

	for (DWORD i = 0; i < m.mapSize; i++) {
		_tprintf(_T("\t|"));
		for (DWORD j = 0; j < m.mapSize; j++) {
			_tprintf(_T("%c"), m.arrMapa[i][j]);
		}
		_tprintf(_T("|\n"));
	}
	_tprintf(_T("\t└"));
	for (DWORD i = 0; i < m.mapSize; i++)_tprintf(_T("─"));
	_tprintf(_T("┘"));
}
TCHAR* verificaComando() {
	TCHAR* tokensP, * nextTokens = NULL, comando[MAXBUFFER];
	TCHAR seps[] = _T(" ,\t\n");
	BOOL sai = FALSE;

	while (sai != TRUE) {
		TCHAR* comandos[MAXBUFFER];
		int n = 0;
		_getts_s(comando, MAXBUFFER);

		tokensP = _tcstok_s(comando, seps, &nextTokens);
		while (tokensP != NULL) {
			comandos[n++] = tokensP;
			tokensP = _tcstok_s(NULL, seps, &nextTokens);
		}
		for (size_t i = 0; i != n; i++) {
			_tprintf(_T("Token %zu is '%s'.\n"), i, comandos[i]);
		}
		//_tprintf(_T("comando:%s\n"), comando,n);
		if (_tcscmp(comandos[0], _T("Bloco")) == 0 && n == 3) {
			if (comandos[1] != NULL && comandos[2] != NULL) {
				if (_wtoi(comandos[1]) > 0 && _wtoi(comandos[2]) > 0) {
					_stprintf_s(comando, 256, _T("%s %s %s"), comandos[0], comandos[1], comandos[2]);
					sai = TRUE;
				}
			}
		}
		else if (_tcscmp(comandos[0], _T("Paraagua")) == 0 && n == 2) {
			if (comandos[1] != NULL && _wtoi(comandos[1]) > 0) {
				_stprintf_s(comando, 256, _T("%s %s"), comandos[0], comandos[1]);
				sai = TRUE;
			}
		}
		else if (_tcscmp(comandos[0], _T("ModoAleatorio")) == 0 && n == 2) {
			if (comandos[1] != NULL) {
				if (_tcscmp(comandos[1], _T("on")) == 0 || _tcscmp(comandos[1], _T("off")) == 0) {
					_stprintf_s(comando, 256, _T("%s %s"), comandos[0], comandos[1]);
					sai = TRUE;
				}
			}
		}
		else if (_tcscmp(comandos[0], _T("Ajuda")) == 0 && n == 1) {
			menuComandosMonitor();
		}
		else {
			_tprintf(_T("ERRO: Digite um comando válido!!!!\n"));
			_tprintf(_T("Caso não se lembre dos comandos digite o comando Ajuda\n"));
		}

		/*_tprintf(_T("comando lido:%s\n"), comando);*/
	}
	return comando;
}
DWORD WINAPI ThreadMonitorLeitura(LPVOID dados) {
	DadosSync* data = (DadosSync*)dados;
	
	while (!data->terminar) {

		WaitForSingleObject(data->hEvent[HEVENT_ESCREVE], INFINITE);
	
		WaitForSingleObject(data->hMutex, INFINITE);
		mostraMapa(data->sharedMem->mapa);
	
		ResetEvent(data->hEvent[HEVENT_ESCREVE]);
		ReleaseMutex(data->hMutex);
		
	}
}
DWORD WINAPI ThreadEncerraMonitor(LPVOID param) {
	DadosSync* dados = (DadosSync*)param;
		WaitForSingleObject(dados->hSem[H_SEM_LOCK], INFINITE);
		
		dados->terminar = 1;
	
}
DWORD WINAPI ThreadProdutor(LPVOID param) {
	DadosSync* dados = (DadosSync*)param;
	BufferCircular cel;
	int i = 0;

	while (!dados->terminar) {
		//_tprintf(_T("\nx:%d,y:%d\n"), dados->sharedMem->buffer[dados->sharedMem->posW].x, dados->sharedMem->buffer[dados->sharedMem->posW].y);
		//esperamos por uma posicao para escrevermos
		WaitForSingleObject(dados->hSem[H_SEM_ESCRITA], INFINITE);
		_tcscpy_s(cel.comando,MAXBUFFER,verificaComando());


		//_tprintf(_T("comando Produzido:%s\n"), cel.comando);
		//esperamos que o mutex esteja livre
		WaitForSingleObject(dados->hMutex, INFINITE);
	
		//vamos copiar a variavel cel para a memoria partilhada (para a posição de escrita)
		CopyMemory(&dados->sharedMem->buffer[dados->sharedMem->posW], &cel, sizeof(BufferCircular));
		
		//_tprintf(_T("Sem released\n"));
		//_tprintf(_T("posR:%d,PosW:%d"), dados->sharedMem->posR, dados->sharedMem->posW);
		//_tprintf(_T("\nx:%d,y:%d\nc:%s\n"), dados->sharedMem->buffer[dados->sharedMem->posW].x, dados->sharedMem->buffer[dados->sharedMem->posW].y, dados->sharedMem->buffer[dados->sharedMem->posW].comando);
		dados->sharedMem->posW++; //incrementamos a posicao de escrita para o proximo produtor escrever na posicao seguinte

		//se apos o incremento a posicao de escrita chegar ao fim, tenho de voltar ao inicio
		if (dados->sharedMem->posW == 5) {
			dados->sharedMem->posW = 0;
		}

		//libertamos o mutex
		ReleaseMutex(dados->hMutex);
		//libertamos o semaforo. temos de libertar uma posicao de leitura
		ReleaseSemaphore(dados->hSem[H_SEM_LEITURA], 1, NULL);
		
	}
	//_tprintf(_T("SAIR ThreadProdutor!\n"));

	return 0;
}
int _tmain(int argc, TCHAR** argv) {
	// definir variáveis
	DadosSync dados;
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif // UNICODE FILE_MAP_READ
	if (!initMemAndSync(&dados)) {
		_tprintf(_T("Erro na inicialização de memória e Sincronização\n"));
		return EXIT_FAILURE;
	}

	HANDLE hThread[3];
	hThread[0] = CreateThread(NULL, 0, ThreadMonitorLeitura, &dados, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, ThreadProdutor, &dados, 0, NULL);
	hThread[2] = CreateThread(NULL, 0, ThreadEncerraMonitor, &dados, 0, NULL);
	if (hThread[0] != NULL && hThread[1] != NULL && hThread[2] != NULL) {
			WaitForMultipleObjects(3, hThread, FALSE, INFINITE);
	}
	//FALTA CLOSEHANDLES E RELEASES
	_tprintf(_T("\n[WARNING] Jogo Terminado com sucesso!!!!\n"));
	WaitForMultipleObjects(2, hThread, FALSE, INFINITE);
	for (size_t i = 0; i < 3; i++)
	{
		CloseHandle(hThread[i]);
	}

	return 0;
}