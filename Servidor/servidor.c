#include "comunicacao.h"
#include <strsafe.h>

DWORD createAttribute(TCHAR* attName, DWORD value, HKEY hkey) {
	if (RegSetValueEx(hkey, // chave
		attName, // nome atributo
		0,			// reservado (0)
		REG_DWORD,		// tipo de registo
		&value,	// valor de atributo
		sizeof(value)) != ERROR_SUCCESS) {
		//_tprintf(_T("%s\n,%d\n,%s"), attName, value, path);
		_tprintf(_T("Erro ao criar atributo 1 %d\n"), GetLastError());
		return ERROR_SET_ATT_VALUE;
	}
}
DWORD createKey(TCHAR* keyName, TCHAR* path, HKEY* hkey) {
	DWORD createReturn;
	_tcscat_s(path, MAXBUFFER, keyName); //TPSO2TUBOS//SERVERKEY
	if (RegCreateKeyEx(HKEY_CURRENT_USER, // handle da chave que procuro
		path,	// subchave
		0,		// 0
		NULL,  // classe
		REG_OPTION_VOLATILE, // opcoes, ou então REG_OPTION_NON_VOLATILE
		KEY_ALL_ACCESS, // o que se pretende com a chave
		NULL, // segurança
		hkey, // onde a posso ir buscar (handle), Handle-> referência de um objeto
		&createReturn // resultado da func, se foi aberta ou criada
	) != ERROR_SUCCESS) { // if error_success quer dizer que a condição é verdadeira
		/*_tprintf(_T("Erro ao criar chave %d!!!"), GetLastError());*/
		return ERROR_CREATING_KEY;
	}
	else {
		if (createReturn == REG_OPENED_EXISTING_KEY) {
			/*_tprintf(_T("Chave existente, foi aberta!!!\n"));*/
			return REG_OPENED_EXISTING_KEY;
		}
		else if (createReturn == REG_CREATED_NEW_KEY) {
			/*_tprintf(_T("Chave nova foi criada!!!\n"));*/
			return REG_CREATED_NEW_KEY;
		}

	}
}
BOOL initMemAndSync(DadosSync* cdata, DWORD mapSize, DWORD waterTime) {
	if (mapSize > 20) {
		_tprintf(_T("Mapa no máximo só pode ter 20x20 células!!!\n"));
		return FALSE;
	}
	if (waterTime <= 0) {
		_tprintf(_T("O tempo não pode ser nulo ou negativo\n"));
		return FALSE;
	}
	cdata->hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		MEMBUFFSIZE,
		SHAREDMEM);

	if (cdata->hMapFile == NULL) {
		_tprintf(_T("Error: CreateFileMapping (%d)\n"), GetLastError());
		return FALSE;
	}
	else if (cdata->hMapFile == ERROR_ALREADY_EXISTS) {
		_tprintf(_T("ABERTO"));
	}


	cdata->sharedMem = (SharedMemory*)MapViewOfFile(cdata->hMapFile,
		FILE_MAP_ALL_ACCESS, // FILE MAP ALL ACCESS, porque nao pode ser só write?????
		0,
		0,
		MEMBUFFSIZE);

	if (cdata->sharedMem == NULL) {
		_tprintf(_T("Error: MapViewOfFile (%d)\n"), GetLastError());
		CloseHandle(cdata->hMapFile);
		return FALSE;
	}

	cdata->sharedMem->posW = 0; // posicao leitura buffer circular
	cdata->sharedMem->posR = 0; // posicao escrita buffer circular
	cdata->sharedMem->mapa.waterTime = waterTime; // tempo de agua
	cdata->sharedMem->mapa.mapSize = mapSize; // mapa siz
	cdata->sharedMem->correAgua = TRUE; // ciclo agua
	cdata->sharedMem->modoPecaAleatorio = FALSE; // modo aleatorio

	cdata->sharedMem->nMonitores = 1;
	cdata->hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);

	if (cdata->hMutex == NULL) {

		_tprintf(_T("ERROR CREATING MUTEX (%d)\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		CloseHandle(cdata->hSem[H_SEM_LOCK]);
		return FALSE;
	}

	cdata->hSem[H_SEM_ESCRITA] = CreateSemaphore(NULL, // att security
		2, // valor inicial
		2, // valor máximo
		SEM_WRITE // semáforo partilhado
	);
	if (cdata->hSem[H_SEM_ESCRITA] == NULL) {
		_tprintf(_T("ERROR CREATING SEMAPHORE WRITE (%d)\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		CloseHandle(cdata->hMutex);
		CloseHandle(cdata->hSem[H_SEM_LOCK]);
		return FALSE;
	}
	cdata->hSem[H_SEM_LEITURA] = CreateSemaphore(NULL, // att security
		0, // valor inicial
		2, // valor máximo
		SEM_READ // semáforo partilhado
	);
	if (cdata->hSem[H_SEM_LEITURA] == NULL) {
		_tprintf(_T("ERROR CREATING SEMAPHORE READ (%d)\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		CloseHandle(cdata->hMutex);
		CloseHandle(cdata->hSem[H_SEM_ESCRITA]);
		CloseHandle(cdata->hSem[H_SEM_LOCK]);
		return FALSE;
	}

	cdata->hEvent[HEVENT_ESCREVE] = CreateEvent(NULL,TRUE,FALSE, EVENT_WRITE);

	if (cdata->hEvent[HEVENT_ESCREVE] == NULL) {
		_tprintf(_T("ERROR CREATING EVENT (%d)\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		CloseHandle(cdata->hMutex);
		CloseHandle(cdata->hSem[H_SEM_LEITURA]);
		CloseHandle(cdata->hSem[H_SEM_ESCRITA]);
		CloseHandle(cdata->hSem[H_SEM_LOCK]);
		return FALSE;
	}
	cdata->hEvent[HEVENT_ESCREVE_CLIENTE] = CreateEvent(NULL, TRUE, FALSE, EVENT_WRITE_CLIENT);

	if (cdata->hEvent[HEVENT_ESCREVE_CLIENTE] == NULL) {
		_tprintf(_T("ERROR CREATING EVENT (%d)\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		CloseHandle(cdata->hMutex);
		CloseHandle(cdata->hSem[H_SEM_LEITURA]);
		CloseHandle(cdata->hSem[H_SEM_ESCRITA]);
		CloseHandle(cdata->hSem[H_SEM_LOCK]);
		return FALSE;
	}
	cdata->hEvent[HEVENT_PAUSE] = CreateEvent(NULL, TRUE, TRUE, EVENT_PAUSE);

	if (cdata->hEvent[HEVENT_PAUSE] == NULL) {

		_tprintf(_T("ERROR CREATING EVENT (%d)\n"), GetLastError());
		UnmapViewOfFile(cdata->sharedMem);
		CloseHandle(cdata->hMapFile);
		CloseHandle(cdata->hMutex);
		CloseHandle(cdata->hSem[H_SEM_LEITURA]);
		CloseHandle(cdata->hSem[H_SEM_ESCRITA]);
		CloseHandle(cdata->hSem[H_SEM_LOCK]);
		CloseHandle(cdata->hEvent[HEVENT_ESCREVE]);
		return FALSE;
	}
		
	return TRUE;
}
void geraOrigem(Mapa* mapa) { // gera ponto inicial e destino


	DWORD ponta = random(4); // qual ponta começa, 4 pontas //y 

	mapa->origemX = ponta;

	DWORD r = random(mapa->mapSize);
	if (1 <= ponta <= 3) {
		while (1) {
			if (r == 0 || r == mapa->mapSize - 1)
				break;
			r = random(mapa->mapSize);
		}
	}

	mapa->origemY = r;
	mapa->destinoX = (mapa->mapSize - 1) - mapa->origemX;
	mapa->destinoY = (mapa->mapSize - 1) - mapa->origemY;
	mapa->arrMapa[mapa->origemX][mapa->origemY] = _T('─');
	mapa->arrMapa[mapa->destinoX][mapa->destinoY] = _T('─');
	_tprintf(_T("x%d y%d\n\n"), mapa->origemX, mapa->origemY);

}
DWORD random(DWORD mapSize) {
	srand(time(NULL));
	return rand() % (mapSize);
}
void mostraMapa(Mapa m) {
	_tprintf(_T("\n\n"));
	for (int i = 0; i < m.mapSize; i++) {
		for (int j = 0; j < m.mapSize; j++) {
			_tprintf(_T("%c"), m.arrMapa[i][j]);
		}
		_tprintf(_T("\n"));
	}
}
void fechaTorneira(int segundos) {
	_tprintf(_T("Fechou a torneira com sucesso! (%d segundos com ela fechada)\n"), segundos);
	Sleep(1000 * segundos);
}
int executaComando(BufferCircular cel, SharedMemory* mem) {
	TCHAR seps[] = _T(" ,\t\n");
	TCHAR* nextTokens, * tokensP = _tcstok_s(cel.comando, seps, &nextTokens);
	DWORD x, y;
	TCHAR* tokens[MAXBUFFER];
	size_t n = 0;
	while (tokensP != NULL) {
		tokens[n++] = tokensP;
		tokensP = _tcstok_s(NULL, seps, &nextTokens);
	}
	for (size_t i = 0; i != n; i++) {
		_tprintf(_T("Token %zu is '%s'.\n"), i, tokens[i]);
	}
	if (_tcscmp(tokens[0], _T("Bloco")) == 0) {
		x = _wtoi(tokens[1]);
		//_tprintf(_T("xTokens:%d"), x);

		y = _wtoi(tokens[2]);
		//_tprintf(_T("yTokens:%d"), y);
		mem->mapa.arrMapa[x][y] = _T('B');
		return 0;
	}
	else if (_tcscmp(tokens[0], _T("Paraagua")) == 0) {
		if (mem->correAgua) {
			mem->correAgua = FALSE;
			int a = _tstoi(tokens[1]);
			_tprintf(_T("a:%d"), a);
			return a;
		}
	}
	else if (_tcscmp(tokens[0], _T("ModoAleatorio")) == 0) {
		if (_tcscmp(tokens[1], _T("on")) == 0) {
			if (!mem->modoPecaAleatorio)
				mem->modoPecaAleatorio = TRUE;
		}
		if (_tcscmp(tokens[1], _T("off")) == 0) {
			if (mem->modoPecaAleatorio)
				mem->modoPecaAleatorio = FALSE;
		}
		return 2;
	}
}
void updateMapa(SharedMemory* mem) {
	unsigned int x = 0, y = 1;
	//_tprintf(_T("update prox %d%d %c\n"), mem->mapa.p.prox[x], mem->mapa.p.prox[y], mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]]);
	if (mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] == _T('═'))
		mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] = _T('─');
	//Atualizar o proximo a cheio, como se verifica o proximo?
	if (mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] == _T('╗'))
		mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] = _T('┐');

	if (mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] == _T('╔'))
		mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] = _T('┌');

	if (mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] == _T('╚'))
		mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] = _T('└');

	if (mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] == _T('╝'))
		mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] = _T('┘');

	if (mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] == _T('║'))
		mem->mapa.arrMapa[mem->mapa.p.prox[x]][mem->mapa.p.prox[y]] = _T('|');

}
DWORD WINAPI ThreadCicloAgua(LPVOID param) {
	DadosSync* dados = (DadosSync*)param;
	int n;
	for (DWORD i = 0; i < dados->sharedMem->mapa.waterTime; i++) {
		_tprintf(_T("Thread Agua a começar em [%d]\n"), dados->sharedMem->mapa.waterTime - i);
		Sleep(1000);
	}

	int x = 0, y = 1;
	dados->sharedMem->mapa.p.atual[x] = dados->sharedMem->mapa.origemX;
	dados->sharedMem->mapa.p.atual[y] = dados->sharedMem->mapa.origemY;
	dados->sharedMem->mapa.p.prox[x] = dados->sharedMem->mapa.origemX;
	dados->sharedMem->mapa.p.prox[y] = dados->sharedMem->mapa.origemY;
	dados->td.dadosCliente.estadoJogo = 0;


	DWORD retorno;
	//_tprintf(_T("origem:%d%d"), dados->sharedMem->mapa.p.atual[x], dados->sharedMem->mapa.p.atual[y]);
	while (1) {
		WaitForSingleObject(dados->hEvent[HEVENT_PAUSE], INFINITE);
		retorno = verificaTubo(dados->sharedMem->mapa, &dados->sharedMem->mapa.p);

		if (retorno == 0) {
			_tprintf(_T("EXISTE TUBO A SEGUIR\n"));

		}
		else if (retorno == 1) {
			_tprintf(_T("PARABÉNS GANHOU O JOGO!!!\n"));
			dados->td.dadosCliente.estadoJogo = 1;
		}
		else if (retorno == -1) {
			_tprintf(_T("GAME OVER!!!!!\n"));
			dados->td.dadosCliente.estadoJogo = -1;
		}
		if (!retorno) {
			WaitForSingleObject(dados->hMutex, INFINITE);
			updateMapa(dados->sharedMem);
			ReleaseMutex(dados->hMutex);
		}

		if (!WriteFile(dados->td.hPipes[0].hPipe, &dados->td.dadosCliente, sizeof(Client), &n, NULL)) {
			_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
			return -1;
		}
		else {
			_tprintf(TEXT("[AGUA] Enviei %d bytes ao leitor [%d]... (WriteFile)\n"), n, 0);

			SetEvent(dados->hEvent[HEVENT_ESCREVE]);
			if (dados->td.dadosCliente.estadoJogo == 1 || dados->td.dadosCliente.estadoJogo == -1) {
				CloseHandle(dados->hThread[1]);
				ResetEvent(dados->hEvent[HEVENT_ESCREVE]);
				break;
			}
		}
		Sleep(1000*dados->td.dadosCliente.tempo);
	}
}
void menuComandosServidor() {
	_tprintf(_T("Começar o jogo ('Iniciar')\n"));
	_tprintf(_T("Lista jogadores e respetiva pontuação ('Lista')\n"));
	_tprintf(_T("\tSuspender e Retomar jogo ('Pause/Resume')\n"));
	_tprintf(_T("\tEncerrar jogo ('Encerrar')\n"));
}
BOOL leComando(DadosSync* dados) {
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
		if (_tcscmp(comandos[0], _T("Iniciar")) == 0 && n == 1) {
			//_tprintf(_T("-----Jogo dos Tubos-----\n"));
			if (dados->td.hPipes->activo) {
				_tprintf(_T("-----Jogo dos Tubos-----\n"));
					ResumeThread(dados->hThread[1]);
				
				
				sai = TRUE;
			}

		}else if (_tcscmp(comandos[0], _T("Lista")) == 0 && n == 1) {
			//listaClientes(dados->sharedMem->mapa);
			sai = TRUE;
		}
		else if (_tcscmp(comandos[0], _T("Pause")) == 0 && n == 1) {
			ResetEvent(dados->hEvent[HEVENT_PAUSE]);

			sai = TRUE;
		}
		else if (_tcscmp(comandos[0], _T("Resume")) == 0 && n == 1) {
			SetEvent(dados->hEvent[HEVENT_PAUSE]);
			sai = TRUE;
		}
		else if (_tcscmp(comandos[0], _T("Encerrar")) == 0 && n == 1) {
			sai = TRUE;
			//return ENCERRA;

		}
		else if (_tcscmp(comandos[0], _T("Ajuda")) == 0 && n == 1) {
			menuComandosServidor();
		}
		else {
			_tprintf(_T("ERRO: Digite um comando válido!!!!\n"));
			_tprintf(_T("Caso não se lembre dos comandos digite o comando Ajuda\n"));
		}

		/*_tprintf(_T("comando lido:%s\n"), comando);*/
	}
	//return comando;
}
DWORD WINAPI ServidorListener(LPVOID param) {
	DadosSync* dados = (DadosSync*)param;
	while (1) {
		leComando(dados);
	}
}
DWORD WINAPI ThreadOuveCliente(LPVOID param) {
	DadosSync* dados = (DadosSync*)param;
	BOOL fSuccess;
	int n;
	Mapa mapa;
	TCHAR pecas[6] = { _T('═'),_T('╗'),_T('║'),_T('╝'),_T('╚'),_T('╔') };
	BOOL start = FALSE;
	while (1) {
	
		fSuccess = ReadFile(
			dados->td.hPipes[0].hPipe,    // pipe handle 
			&dados->td.dadosCliente,    // buffer to receive reply 
			sizeof(Client),  // size of buffer 
			&n,  // number of bytes read 
			NULL);    // not overlapped 

		if (dados->td.dadosCliente.estadoJogo == 1) {
			WaitForSingleObject(dados->hEvent[HEVENT_ESCREVE_CLIENTE], INFINITE);
			ResetEvent(dados->hEvent[HEVENT_ESCREVE_CLIENTE]);
			carregaMapa(&dados->sharedMem->mapa);
			dados->td.dadosCliente.tempo -= 1000;
			dados->hThread[1] = CreateThread(NULL, 0, ThreadCicloAgua, dados, 0, NULL);
			_tprintf(_T("CONTINUEI\n"));
			dados->td.dadosCliente.estadoJogo = 0;
			dados->td.dadosCliente.nivel++;
			dados->td.dadosCliente.pontos += 10;
			mostraMapa(dados->sharedMem->mapa);
		}else if (dados->td.dadosCliente.estadoJogo == -1) {
			WaitForSingleObject(dados->hEvent[HEVENT_ESCREVE_CLIENTE], INFINITE);
			ResetEvent(dados->hEvent[HEVENT_ESCREVE_CLIENTE]);
			carregaMapa(&dados->sharedMem->mapa);
			dados->hThread[1] = CreateThread(NULL, 0, ThreadCicloAgua, dados, 0, NULL);
			_tprintf(_T("RECOMECEI\n"));
			dados->td.dadosCliente.estadoJogo = 0;
			dados->td.dadosCliente.nivel=0;
			dados->td.dadosCliente.pontos =0;
			mostraMapa(dados->sharedMem->mapa);
		}
		else if (_tcscmp(dados->td.dadosCliente.nome, _T("joga")) == 0) {
			WaitForSingleObject(dados->hMutex, INFINITE);
			if (!jogarPeca(dados->td.dadosCliente.xPos, dados->td.dadosCliente.yPos, &dados->sharedMem->mapa, &pecas))
				_tprintf(TEXT("[AVISO] Não atualizou mapa!!!\n"));
			dados->td.dadosCliente.xPos = -1;
			dados->td.dadosCliente.yPos = -1;
			ReleaseMutex(dados->hMutex);
		}
		WaitForSingleObject(dados->hMutex, INFINITE);
		dados->td.dadosCliente.mapa = dados->sharedMem->mapa;
		ReleaseMutex(dados->hMutex);
		if (!WriteFile(dados->td.hPipes[0].hPipe, &dados->td.dadosCliente, sizeof(Client), &n, NULL)) {
			_tprintf(TEXT("[ERRO] Escrever no pipe! (WriteFile)\n"));
			return -1;
		}
		else {
			_tprintf(TEXT("[ESCRITOR] Enviei %d bytes ao leitor [%d]... (WriteFile)\n"), n, 0);

			SetEvent(dados->hEvent[HEVENT_ESCREVE]);
		}
	}
	
	_tprintf(_T("SAINDO\n"));
	
	return 0;
}
DWORD WINAPI ThreadConsumidor(LPVOID param) {
	DadosSync* dados = (DadosSync*)param;
	BufferCircular cel;
	int valor = 0;


	while (dados->terminar != 1) {
		WaitForSingleObject(dados->hEvent[HEVENT_PAUSE], INFINITE);
		WaitForSingleObject(dados->hSem[H_SEM_LEITURA], INFINITE);

		WaitForSingleObject(dados->hMutex, INFINITE);

		CopyMemory(&cel, &dados->sharedMem->buffer[dados->sharedMem->posR], sizeof(BufferCircular));
		valor = executaComando(cel, dados->sharedMem);
		if (!dados->sharedMem->correAgua) {
			ResetEvent(dados->hEvent[HEVENT_PAUSE]);
			fechaTorneira(valor);
			dados->sharedMem->correAgua = TRUE;
			SetEvent(dados->hEvent[HEVENT_PAUSE]);
		}

		//_tprintf(_T("\nxs:%d,ys:%d\ncomando:%s"), dados->sharedMem->buffer[dados->sharedMem->posR].x, dados->sharedMem->buffer[dados->sharedMem->posR].y, dados->sharedMem->buffer[dados->sharedMem->posR].comando);
		dados->sharedMem->posR++; //incrementamos a posicao de leitura para o proximo consumidor ler na posicao seguinte
		//se apos o incremento a posicao de leitura chegar ao fim, tenho de voltar ao inicio
		if (dados->sharedMem->posR == 5)
			dados->sharedMem->posR = 0;

		//libertamos o mutex
		ReleaseMutex(dados->hMutex);

		//libertamos o semaforo. temos de libertar uma posicao de escrita
		ReleaseSemaphore(dados->hSem[H_SEM_ESCRITA], 1, NULL);

	}

	return 0;
}
TCHAR escolheTubo(BOOL value, TCHAR* pecas) {

	//// ╚ ╝ ║ ╗  ═
	////	╔
	int r = 0;
	if (value) {
		r = rand() % 6;
		return pecas[r];
	}
	TCHAR peca;
	_tprintf(_T("\n"));
	for (int i = 0; i < 6; i++) {//n pecas
		_tprintf(_T("%d:%c\n"), i, pecas[i]);
	}
	peca = pecas[0];
	for (int i = 0; i < 6; i++) {// ordena pecas
		if (i < 5) {
			pecas[i] = pecas[i + 1];
			pecas[i + 1] = peca;
		}
	}

	_tprintf(_T("\n\n%c\n\n"), peca);
	return peca;
}
BOOL jogarPeca(int x, int y, Mapa *mapa,TCHAR * pecas){
	//_tprintf(_T("x:%d y:%d"), x, y);
	for (int i = 0; i < mapa->mapSize; i++)
	{
		for (int j = 0; j < mapa->mapSize; j++)
		{
			if (x == j && y == i) {
				// saber se já tem água 
				if (mapa->arrMapa[i][j] == 9472 || mapa->arrMapa[i][j] == 9488 || mapa->arrMapa[i][j] == 9484
					|| mapa->arrMapa[i][j] == 124 || mapa->arrMapa[i][j] == 9492 || mapa->arrMapa[i][j] == 9496) {
					return FALSE;

				}
				//_tprintf(_T("1x:%d y:%d i:%d j:%d"), x, y, i, j);
				TCHAR c;
				if (mapa->modoAleatorio) {
					c = escolheTubo(TRUE, pecas);
					return TRUE;
				}
				else {
					c = escolheTubo(FALSE, pecas);
				}
				
				mapa->arrMapa[i][j] = c;
				//mostraMapa(*mapa);
				return TRUE;
			}

		}
	}
	return FALSE;
}
void carregaMapa(Mapa* mapa) {
	//mapa->mapSize = 4;
	//mapa->origemX = 0;
	//mapa->origemY = 0;
	//mapa->destinoX = 3;
	//mapa->destinoY = 3;
	
	_tprintf(_T("s:%d\nx:%d y:%d\n\nx1:%d y1:%d\n "), mapa->mapSize, mapa->origemX, mapa->origemY, mapa->destinoX, mapa->destinoY);
	//mapa->arrMapa[mapa->origemX][mapa->origemY] = _T('─');
	//mapa->arrMapa[mapa->destinoX][mapa->destinoY] = _T('─');
	for (int i = 0; i < mapa->mapSize; i++) {
		for (int j = 0; j < mapa->mapSize; j++) {
			if (mapa->arrMapa[i][j] == NULL)
				mapa->arrMapa[i][j] = _T('x');
		}

	}
	geraOrigem(mapa);
	
	
	//mapa->arrMapa[0][1] = _T('┐');
	//mapa->arrMapa[0][2] = _T('┌');
	//mapa->arrMapa[0][3] = _T('┐');
	//mapa->arrMapa[1][0] = _T('x');
	// ╚ ╝ ║ ╗  ═
	//	╔
	/*mapa->arrMapa[1][1] = _T('└');
	mapa->arrMapa[1][2] = _T('┘');
	mapa->arrMapa[1][3] = _T('|');
	mapa->arrMapa[2][0] = _T('─');
	mapa->arrMapa[2][1] = _T('|');

	mapa->arrMapa[2][2] = _T('┌');
	mapa->arrMapa[2][3] = _T('┘');
	mapa->arrMapa[3][0] = _T('x');
	mapa->arrMapa[3][1] = _T('└');

	mapa->arrMapa[3][2] = _T('└');
	 */
	//mapa->arrMapa[0][1] = _T('╗');
	//mapa->arrMapa[0][2] = _T('╔');
	//mapa->arrMapa[0][3] = _T('╗');
	//mapa->arrMapa[1][0] = _T('x');
	//// ╚ ╝ ║ ╗  ═
	////	╔
	//mapa->arrMapa[1][1] = _T('╚');
	//mapa->arrMapa[1][2] = _T('╝');
	//mapa->arrMapa[1][3] = _T('║');
	//mapa->arrMapa[2][0] = _T('═');
	//mapa->arrMapa[2][1] = _T('║');

	//mapa->arrMapa[2][2] = _T('╔');
	//mapa->arrMapa[2][3] = _T('╝');
	//mapa->arrMapa[3][0] = _T('x');
	//mapa->arrMapa[3][1] = _T('╚');

	//mapa->arrMapa[3][2] = _T('╚');
	
}
DWORD verificaPosicao(Mapa m, Posicao p) {
	unsigned int x = 0, y = 1;
	// Se foi para a direita
	//_tprintf(_T("posicao %d\n"), p.direcao);
	if (p.direcao == 0) {// Se foi para a esquerda _T('═') _T('╚') _T('╔')
		if (m.arrMapa[p.atual[x]][p.atual[y] - 1] == 9552
			|| m.arrMapa[p.atual[x]][p.atual[y] - 1] == 9562
			|| m.arrMapa[p.atual[x]][p.atual[y] - 1] == 9556
			) {
			//_tprintf(_T("fui esquerda %d%d\n"), p.atual[x], p.atual[y] - 1);
			return p.atual[y] - 1;
		}
	}
	else if (p.direcao == 1) { // se foi para cima _T('║') _T('╗') _T('╔')
		//_tprintf(_T("atualver3\n"));
		if (m.arrMapa[p.atual[x] - 1][p.atual[y]] == 9553
			|| m.arrMapa[p.atual[x] - 1][p.atual[y]] == 9559
			|| m.arrMapa[p.atual[x] - 1][p.atual[y]] == 9556
			) {
			//_tprintf(_T("fui cima %d%d\n"), p.atual[x] - 1, p.atual[y]);
			return p.atual[x] - 1;
		}
	}
	else if (p.direcao == 2) {// Se foi para a direita _T('═') _T('╗') _T('╝') _T('─')
		if (m.arrMapa[p.atual[x]][p.atual[y] + 1] == 9552
			|| m.arrMapa[p.atual[x]][p.atual[y] + 1] == 9559
			|| m.arrMapa[p.atual[x]][p.atual[y] + 1] == 9565
			|| m.arrMapa[p.atual[x]][p.atual[y] + 1] == 9472
			) {
			_tprintf(_T("fui direita %d%d\n"), p.atual[x], p.atual[y] + 1);
			return p.atual[y] + 1;
		}

	}
	 
	// se foi para baixo _T('╝') _T('║') _T('╚')
	else if (p.direcao == 3) {
		if (m.arrMapa[p.atual[x] + 1][p.atual[y]] == 9565
			|| m.arrMapa[p.atual[x] + 1][p.atual[y]] == 9553
			|| m.arrMapa[p.atual[x] + 1][p.atual[y]] == 9562

			) {
			//_tprintf(_T("fui baixo %d%d\n"), p.atual[x] + 1, p.atual[y]);
			return p.atual[x] + 1;
		}
	}
	return -1;
}
DWORD verificaTubo(Mapa m, Posicao* p) {
	int x = 0, y = 1;
	_tprintf(_T("NICE8 atual: %d %d\nprox: %d %d"), p->atual[x], p->atual[y], p->prox[x], p->prox[y]);
	//_tprintf(_T("NICE8 atual: %d %d\ndest: %d %d"), p->atual[x], p->prox[x] , m.destinoX, m.destinoY);
	mostraMapa(m);
	if (p->atual[x] == m.destinoX && p->atual[y] == m.destinoY) {
		p->atual[x] = 0;
		p->atual[y] = 0;
		p->prox[x] = 0;
		p->prox[y] = 0;
		return 1;
	}
		
	if (m.arrMapa[p->atual[x]][p->atual[y]] == 9472) {//_T('─')
		// se andou pra esquerda
	  _tprintf(_T("\n\n─ atual: %d%d\n\n"), p->atual[x], p->atual[y]);
		p->direcao = 0;	
		if (verificaPosicao(m, *p) != -1) {
			p->prox[y] = verificaPosicao(m, *p);
			p->atual[y] = p->prox[y];
			_tprintf(_T("esquerda prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}
		// se andou pra direita
		p->direcao = 2;
		if (verificaPosicao(m, *p) != -1) {
			p->prox[y] = verificaPosicao(m, *p);
			p->atual[y] = p->prox[y];
			_tprintf(_T("direita prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}

	}
	else if (m.arrMapa[p->atual[x]][p->atual[y]] == 9488) {//_T('┐')
		// se andou pra baixo
		p->direcao = 3;
		_tprintf(_T("\n\n┐ atual: %d%d\n\n"), p->atual[x], p->atual[y]);

		if (verificaPosicao(m, *p) != -1) {
			p->prox[x] = verificaPosicao(m, *p);
			p->atual[x] = p->prox[x];
			_tprintf(_T("baixo prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}

		// se foi pra esquerda
		p->direcao = 0;

		if (verificaPosicao(m, *p) != -1) {
			p->prox[y] = verificaPosicao(m, *p);
			p->atual[y] = p->prox[y];
			_tprintf(_T("esquerda prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}

	}
	else if (m.arrMapa[p->atual[x]][p->atual[y]] == 9484) {//_T('┌')
		// se andou pra direita
		p->direcao = 2;
		_tprintf(_T("\n\n┌ atual: %d%d\n\n"), p->atual[x], p->atual[y]);

		if (verificaPosicao(m, *p) != -1) {
			p->prox[y] = verificaPosicao(m, *p);
			p->atual[y] = p->prox[y];
			_tprintf(_T("direita prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}
		// se foi pra baixo
		p->direcao = 3;

		if (verificaPosicao(m, *p) != -1) {
			p->prox[x] = verificaPosicao(m, *p);
			p->atual[x] = p->prox[x];
			_tprintf(_T("baixo prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}

	}
	else if (m.arrMapa[p->atual[x]][p->atual[y]] == 9492) {//_T('└')
		// se andou pra direita
		p->direcao = 2;
		_tprintf(_T("\n\n└ atual: %d%d\n\n"), p->atual[x], p->atual[y]);
		if (verificaPosicao(m, *p) != -1) {
			p->prox[y] = verificaPosicao(m, *p);
			p->atual[y] = p->prox[y];
			_tprintf(_T("direita prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}

		// se andou pra cima
		p->direcao = 1;

		if (verificaPosicao(m, *p) != -1) {
			p->prox[x] = verificaPosicao(m, *p);
			p->atual[x] = p->prox[x];
			//_tprintf(_T("cima prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}

	}
	else if (m.arrMapa[p->atual[x]][p->atual[y]] == 9496) {//_T('┘')
		// se andou para cima
		p->direcao = 1;
		//_tprintf(_T("\n\n┘ atual: %d%d\n\n"), p->atual[x], p->atual[y]);
		if (verificaPosicao(m, *p) != -1) {
			p->prox[x] = verificaPosicao(m, *p);
			p->atual[x] = p->prox[x];
			//_tprintf(_T("cima prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}

		// se andou pra esquerda
		p->direcao = 0;

		if (verificaPosicao(m, *p) != -1) {
			p->prox[y] = verificaPosicao(m, *p);
			p->atual[y] = p->prox[y];
			//_tprintf(_T("esquerda prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}

	}
	else if (m.arrMapa[p->atual[x]][p->atual[y]] == 124 ) {//_T('|')
		// se andou pra baixo
		p->direcao = 3;
		//_tprintf(_T("\n\n| atual: %d%d\n"), p->atual[x], p->atual[y]);
		if (verificaPosicao(m, *p) != -1) {
			p->prox[x] = verificaPosicao(m, *p);
			p->atual[x] = p->prox[x];
			//_tprintf(_T("baixo prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}
		// se andou pra cima
		p->direcao = 1;

		if (verificaPosicao(m, *p) != -1) {
			p->prox[x] = verificaPosicao(m, *p);
			p->atual[x] = p->prox[x];
			//_tprintf(_T("cima prox: %d%d\n\n"), p->prox[x], p->prox[y]);
			return 0;
		}
	}
	return -1;
}

int _tmain(int argc, TCHAR** argv) {
	DadosSync dados;
	HKEY hGamekey;
	TCHAR path[256] = _T("Software\\TPSO2TUBOS\\"), comando[256];
	DWORD mapSize = 0;
	DWORD waterTime = 0;
	BOOL terminar = FALSE;
	ThreadData dadosThread;
	TCHAR chave_nome[MAXBUFFER], nome[MAXBUFFER], valor[MAXBUFFER];
	TCHAR buf[256];
	DWORD sizeNome, sizeValor;
	HANDLE hPipe, hEventTemp;
	int i,n, numClientes = 0;
	DWORD offset, nBytes;
	Client mc;
	BOOL fSuccess;
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif // UNICODE
	dados.hSem[H_SEM_LOCK] = CreateSemaphore(NULL, 1, 1, SEM_SERVER_LOCK);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(_T("Já existe uma instância servidor a correr\n"));
		return EXISTING_SERVER;
	}
	WaitForSingleObject(dados.hSem[H_SEM_LOCK], INFINITE);

	if (argc < 3) {
		_tcscat_s(path, 256, _T("SERVERKEY\\"));

		if (RegCreateKeyEx(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hGamekey, NULL) != ERROR_SUCCESS)
			return EXIT_FAILURE;

		RegQueryValueEx(hGamekey, WATER_ATT_NAME, NULL, NULL, (LPBYTE)&waterTime, &sizeNome);
		RegQueryValueEx(hGamekey, MAP_ATT_NAME, NULL, NULL, (LPBYTE)&mapSize, &sizeValor);

	}
	else {
		mapSize = (DWORD)_tstoi(argv[1]);
		waterTime = (DWORD)_tstoi(argv[2]);
		if (createKey(SERVER_KEY_NAME, path, &hGamekey) == ERROR_CREATING_KEY)
			return ERROR_CREATING_KEY;

		createAttribute(MAP_ATT_NAME, mapSize, hGamekey);
		createAttribute(WATER_ATT_NAME, waterTime, hGamekey);
	}
	
	dados.terminar = 0;

	if (!initMemAndSync(&dados, mapSize, waterTime)) {
			_tprintf(_T("Erro na inicialização de memória e Sincronização\n"));
			return EXIT_FAILURE;
	}

	_tprintf(_T("Info:\n\tTamanho do mapa [%d]x[%d]\n\tTempo de água ate fluir [%d]\n\n"), dados.sharedMem->mapa.mapSize, dados.sharedMem->mapa.mapSize, dados.sharedMem->mapa.waterTime);


	carregaMapa(&dados.sharedMem->mapa);
		//_tprintf(_T("dados.sharedMem->mapa[0][0]= %c\n"), dados.sharedMem->mapa.arrMapa[0][0]);
		// ╚ ╝ ║ ╗  ═
		//	╔
		//mostraMapa(dados.sharedMem->mapa);


		for (i = 0; i < 2; i++) {

	
			hPipe = CreateNamedPipe(PIPE_SERVER, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
				PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
				N,
				256 * sizeof(TCHAR),
				256 * sizeof(TCHAR),
				1000,
				NULL);

			if (hPipe == INVALID_HANDLE_VALUE) {
				_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)"));

			}
			


			// criar evento que vai ser associado à esturtura overlaped
			// os eventos aqui tem de ter sempre reset manual e nao automático porque temos de delegar essas responsabilidades ao sistema operativo
			hEventTemp = CreateEvent(NULL, TRUE, FALSE, NULL);

			if (hEventTemp == NULL) {
				_tprintf(TEXT("[ERRO] ao criar evento\n"));
				return -1;
			}

			dados.td.hPipes[i].hPipe = hPipe;
			dados.td.hPipes[i].activo = FALSE;
			//temos de garantir que a estrutura overlap está limpa
			ZeroMemory(&dados.td.hPipes[i].overlap, sizeof(dados.td.hPipes[i].overlap));
			//preenchemos agora o evento
			dados.td.hPipes[i].overlap.hEvent = hEventTemp;
			dados.td.hEventos[i] = hEventTemp;
			_tprintf(TEXT("[ESCRITOR] Esperar ligação de um leitor... (ConnectNamedPipe)\n"));
			DWORD dwTid;

			ConnectNamedPipe(dados.td.hPipes[i].hPipe, &dados.td.hPipes[i].overlap);
			_tprintf(_T("CONNECTEI"));

		}

		BOOL start = FALSE;
		while (numClientes < 2) {
			//permite estar bloqueado , à espera que 1 evento do array de enventos seja assinalado
			offset = WaitForMultipleObjects(2, dados.td.hEventos, FALSE, INFINITE);
			i = offset - WAIT_OBJECT_0; // devolve o indice da instancia do named pipe que está ativa, aqui sabemos em que indice o cliente se ligou

			// se é um indice válido ...
			if (i >= 0 && i < 2) {

				_tprintf(TEXT("[ESCRITOR] Leitor %d chegou\n"), i);
				if (!start) {
					dados.hThread[0] = CreateThread(NULL, 0, ThreadOuveCliente, &dados, 0, NULL);
					WaitForSingleObject(dados.hEvent[HEVENT_ESCREVE_CLIENTE],INFINITE);
					dados.td.dadosCliente.tempo = 10000;
					dados.hThread[1] = CreateThread(NULL, 0, ThreadCicloAgua, &dados, CREATE_SUSPENDED, NULL);
					dados.hThread[2] = CreateThread(NULL, 0, ThreadConsumidor, &dados, 0, NULL);
					dados.hThread[3] = CreateThread(NULL, 0, ServidorListener, &dados, 0, NULL);// porque nao termina
					if (dados.hThread[0] == NULL && dados.hThread[1] == NULL && dados.hThread[2] == NULL && dados.hThread[3] == NULL) {
						_tprintf(_T("ERRO CRIACAO THREAD"));
					}
					start = TRUE;
				}
				
				if (GetOverlappedResult(dados.td.hPipes[i].hPipe, &dados.td.hPipes[i].overlap, &nBytes, FALSE)) {
					// se entrarmos aqui significa que a funcao correu tudo bem
					// fazemos reset do evento porque queremos que o WaitForMultipleObject desbloqueio com base noutro evento e nao neste
					ResetEvent(dados.td.hEventos[i]);

					//vamos esperar que o mutex esteja livre
					WaitForSingleObject(dados.hMutex, INFINITE);
					dados.td.hPipes[i].activo = TRUE; // dizemos que esta instancia do named pipe está ativa
					ReleaseMutex(dados.hMutex);

					numClientes++;
				}
			}
		}


		WaitForMultipleObjects(4, dados.hThread, TRUE, INFINITE);
		ReleaseSemaphore(dados.hSem[H_SEM_ESCRITA], 1, NULL);
		CloseHandle(dados.hEvent[HEVENT_ESCREVE]);
		CloseHandle(dados.hEvent[HEVENT_PAUSE]);
		CloseHandle(dados.hMapFile);
		UnmapViewOfFile(dados.sharedMem);
		CloseHandle(dados.hMutex);
		for (int i = 0; i < 2; i++) {
			_tprintf(TEXT("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));

			//desliga todas as instancias de named pipes
			if (!DisconnectNamedPipe(dados.td.hPipes[i].hPipe)) {
				_tprintf(TEXT("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));

				exit(-1);
			}
		}

		return 0;

}
