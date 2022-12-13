// clientegui.cpp : Define o ponto de entrada para o aplicativo.
//

#include "framework.h"
#include "clientegui.h"

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <Windows.h>
#include <conio.h>
#include "..//Servidor//comunicacao.h"

#define MODO_COMPETITIVO 100
#define MODO_INDIVIDUAL 200
#define ESTADO_INICIAL -1
/* ===================================================== */
/* Programa base (esqueleto) para aplicações Windows     */
/* ===================================================== */
// Cria uma janela de nome "Janela Principal" e pinta fundo de branco
// Modelo para programas Windows:
//  Composto por 2 funções: 
//	WinMain()     = Ponto de entrada dos programas windows
//			1) Define, cria e mostra a janela
//			2) Loop de recepção de mensagens provenientes do Windows
//     TrataEventos()= Processamentos da janela (pode ter outro nome)
//			1) É chamada pelo Windows (callback) 
//			2) Executa código em função da mensagem recebida

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);

// Nome da classe da janela (para programas de uma só janela, normalmente este nome é 
// igual ao do próprio programa) "szprogName" é usado mais abaixo na definição das 
// propriedades do objecto janela
TCHAR szProgName[] = TEXT("Base");

HWND InitBox, NomeBox, CompetitivoBox, IndividualBox;
HANDLE hPipe, hEvento, hThread, hMutex;
HDC hdc;
HDC bmpDC;
HWND hWnd;
DadosSync dados;
OVERLAPPED ov, overlap;
static int aux = 0;
static Client dadosCliente;
// ============================================================================
// FUNÇÃO DE INÍCIO DO PROGRAMA: WinMain()
// ============================================================================
// Em Windows, o programa começa sempre a sua execução na função WinMain()que desempenha
// o papel da função main() do C em modo consola WINAPI indica o "tipo da função" (WINAPI
// para todas as declaradas nos headers do Windows e CALLBACK para as funções de
// processamento da janela)
// Parâmetros:
//   hInst: Gerado pelo Windows, é o handle (número) da instância deste programa 
//   hPrevInst: Gerado pelo Windows, é sempre NULL para o NT (era usado no Windows 3.1)
//   lpCmdLine: Gerado pelo Windows, é um ponteiro para uma string terminada por 0
//              destinada a conter parâmetros para o programa 
//   nCmdShow:  Parâmetro que especifica o modo de exibição da janela (usado em  
//        	   ShowWindow()
DWORD WINAPI OuveServidor(LPVOID lparam)
{
	DadosSync* dados = (DadosSync*)lparam;
	DWORD n;

	OVERLAPPED overlap;
	HANDLE hEvent;
	BOOL fSuccess = FALSE;
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	static int count = 0;
	while (1) {
		WaitForSingleObject(dados->hEvent[HEVENT_ESCREVE], INFINITE);
		//ResetEvent(dados->hEvent[HEVENT_ESCREVE_CLIENTE]);
		//MessageBox(hWnd, _T("OuveServidor"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
		fSuccess = FALSE;
		Sleep(500);
		ZeroMemory(&overlap, sizeof(overlap));
		overlap.hEvent = hEvent;
		//WaitForSingleObject(hMutex, INFINITE);
		fSuccess = ReadFile(
			hPipe,    // pipe handle 
			&dadosCliente,    // buffer to receive reply 
			sizeof(Client),  // size of buffer 
			&n,  // number of bytes read 
			&overlap);    //  overlapped 

		//ReleaseMutex(hMutex);

		if (fSuccess || GetLastError() == ERROR_IO_PENDING) {
			//MessageBox(hWnd, _T("OuveServidor2"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
			WaitForSingleObject(ov.hEvent, INFINITE);
			GetOverlappedResult(hPipe, &overlap, &n, FALSE);
			if (dadosCliente.estadoJogo == 1) {
				MessageBox(hWnd, _T("GANHOU\n"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
				Sleep(2000);
				SetEvent(dados->hEvent[HEVENT_ESCREVE_CLIENTE]);
			}
			else if (dadosCliente.estadoJogo == -1) {
				MessageBox(hWnd, _T("GAME OVER\n"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
				Sleep(2000);
				SetEvent(dados->hEvent[HEVENT_ESCREVE_CLIENTE]);
			}
			InvalidateRect(hWnd, NULL, TRUE); // requisita WM_PAINT
		}
	}

	return 0;
}
int WINAPI WinMain(HINSTANCE hInst, // instancia atual app
	HINSTANCE hPrevInst,//
	LPSTR lpCmdLine, int nCmdShow) {
	// hWnd é o handler da janela, gerado mais abaixo por CreateWindow()
	MSG lpMsg;		// MSG é uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp;	// WNDCLASSEX é uma estrutura cujos membros servem para 
			  // definir as características da classe da janela

	_tprintf(TEXT("[LEITOR] Ligação ao pipe do escritor... (CreateFile)\n"));

	//ligamo-nos ao named pipe que ja existe nesta altura
	//1º nome do named pipe, 2ºpermissoes (têm de ser iguais ao CreateNamedPipe do servidor), 3ºshared mode 0 aqui,
	//4º security atributes, 5ºflags de criação OPEN_EXISTING, 6º o default é FILE_ATTRIBUTE_NORMAL e o 7º é o template é NULL
	while (1) {
		hPipe = CreateFile(PIPE_SERVER, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

		if (hPipe == INVALID_HANDLE_VALUE) {
			MessageBox(hWnd, _T("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n", PIPE_SERVER), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
			return FALSE;
		}
		else {
			//MessageBox(hWnd, _T("[LEITOR] Liguei-me..."), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
			break;
		}
		if (!WaitNamedPipe(PIPE_SERVER, NMPWAIT_WAIT_FOREVER)) {
			MessageBox(hWnd, _T("[ERRO] Ligar ao pipe !(WaitNamedPipe)\n"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
			return FALSE;
		}

	}
	dados.hEvent[HEVENT_ESCREVE] = OpenEvent(EVENT_ALL_ACCESS, TRUE, EVENT_WRITE);
	dados.hEvent[HEVENT_ESCREVE_CLIENTE] = OpenEvent(EVENT_ALL_ACCESS, TRUE, EVENT_WRITE_CLIENT);
	hThread = CreateThread(NULL, 0, OuveServidor, (LPVOID)&dados, 0, NULL);

	// All pipe instances are busy, so wait for 20 seconds. 

	//MessageBox(*hWnd, _T("[ERRO] Ligar ao pipe !(WaitNamedPipe)1\n"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
	hEvento = CreateEvent(NULL, TRUE, FALSE, NULL);
	hMutex = OpenMutex(NULL, FALSE, MUTEX_NAME);
	// ============================================================================
	// 1. Definição das características da janela "wcApp" 
	//    (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
	// ============================================================================
	wcApp.cbSize = sizeof(WNDCLASSEX);      // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst;		         // Instância da janela actualmente exibida 
								   // ("hInst" é parâmetro de WinMain e vem 
										 // inicializada daí)
	wcApp.lpszClassName = szProgName;       // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = TrataEventos;       // Endereço da função de processamento da janela
											// ("TrataEventos" foi declarada no início e
											// encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;  // Estilo da janela: Fazer o redraw se for
											// modificada horizontal ou verticalmente

	wcApp.hIcon = LoadIcon(NULL, IDI_WARNING);   // "hIcon" = handler do ícon normal
										   // "NULL" = Icon definido no Windows
										   // "IDI_AP..." Ícone "aplicação"
	wcApp.hIconSm = LoadIcon(NULL, IDI_WARNING); // "hIconSm" = handler do ícon pequeno
										   // "NULL" = Icon definido no Windows
										   // "IDI_INF..." Ícon de informação
	wcApp.hCursor = LoadCursor(NULL, IDC_HAND);	// "hCursor" = handler do cursor (rato) 
							  // "NULL" = Forma definida no Windows
							  // "IDC_ARROW" Aspecto "seta" 
	wcApp.lpszMenuName = NULL;			// Classe do menu que a janela pode ter
							  // (NULL = não tem menu)
	wcApp.cbClsExtra = 0;				// Livre, para uso particular
	wcApp.cbWndExtra = 0;				// Livre, para uso particular
	wcApp.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por
	// "GetStockObject".Neste caso o fundo será branco

	// ============================================================================
	// 2. Registar a classe "wcApp" no Windows
	// ============================================================================
	if (!RegisterClassEx(&wcApp))
		return(0);

	// ============================================================================
	// 3. Criar a janela
	// ============================================================================
	hWnd = CreateWindow(
		szProgName,			// Nome da janela (programa) definido acima
		TEXT("TPSO2"),// Texto que figura na barra do título
		WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,	// Estilo da janela (WS_OVERLAPPED= normal)
		600,		// Posição x pixels (default=à direita da última) 
		100,		// Posição y pixels (default=abaixo da última)
		800,		// Largura da janela (em pixels)
		600,		// Altura da janela (em pixels)
		(HWND)HWND_DESKTOP,	// handle da janela pai (se se criar uma a partir de
						// outra) ou HWND_DESKTOP se a janela for a primeira, 
						// criada a partir do "desktop"
		(HMENU)NULL,			// handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst,		// handle da instância do programa actual ("hInst" é 
						// passado num dos parâmetros de WinMain()
		0);				// Não há parâmetros adicionais para a janela
	  // ============================================================================
	  // 4. Mostrar a janela
	  // ============================================================================
	ShowWindow(hWnd, nCmdShow);	// "hWnd"= handler da janela, devolvido por 
					  // "CreateWindow"; "nCmdShow"= modo de exibição (p.e. 
					  // normal/modal); é passado como parâmetro de WinMain()
	UpdateWindow(hWnd);		// Refrescar a janela (Windows envia à janela uma 
					  // mensagem para pintar, mostrar dados, (refrescar)… 
	// ============================================================================
	// 5. Loop de Mensagens
	// ============================================================================
	// O Windows envia mensagens às janelas (programas). Estas mensagens ficam numa fila de
	// espera até que GetMessage(...) possa ler "a mensagem seguinte"	
	// Parâmetros de "getMessage":
	// 1)"&lpMsg"=Endereço de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no  
	//   início de WinMain()):
	//			HWND hwnd		handler da janela a que se destina a mensagem
	//			UINT message		Identificador da mensagem
	//			WPARAM wParam		Parâmetro, p.e. código da tecla premida
	//			LPARAM lParam		Parâmetro, p.e. se ALT também estava premida
	//			DWORD time		Hora a que a mensagem foi enviada pelo Windows
	//			POINT pt		Localização do mouse (x, y) 
	// 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
	//   receber as mensagens para todas as
	// janelas pertencentes à thread actual)
	// 3)Código limite inferior das mensagens que se pretendem receber
	// 4)Código limite superior das mensagens que se pretendem receber

	// NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
	// 	  terminando então o loop de recepção de mensagens, e o programa 

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);	// Pré-processamento da mensagem (p.e. obter código 
					   // ASCII da tecla premida)
		DispatchMessage(&lpMsg);	// Enviar a mensagem traduzida de volta ao Windows, que
					   // aguarda até que a possa reenviar à função de 
					   // tratamento da janela, CALLBACK TrataEventos (abaixo)
	}

	// ============================================================================
	// 6. Fim do programa
	// ============================================================================
	return((int)lpMsg.wParam);	// Retorna sempre o parâmetro wParam da estrutura lpMsg
}

// ============================================================================
// FUNÇÃO DE PROCESSAMENTO DA JANELA
// Esta função pode ter um nome qualquer: Apenas é necesário que na inicialização da
// estrutura "wcApp", feita no início de // WinMain(), se identifique essa função. Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pré-processadas
// no loop "while" da funçãvo WinMain()
// Parâmetros:
//		hWnd	O handler da janela, obtido no CreateWindow()
//		messg	Ponteiro para a estrutura mensagem (ver estrutura em 5. Loop...
//		wParam	O parâmetro wParam da estrutura messg (a mensagem)
//		lParam	O parâmetro lParam desta mesma estrutura
//
// NOTA:Estes parâmetros estão aqui acessíveis o que simplifica o acesso aos seus valores
//
// A função EndProc é sempre do tipo "switch..." com "cases" que descriminam a mensagem
// recebida e a tratar.
// Estas mensagens são identificadas por constantes (p.e. 
// WM_DESTROY, WM_CHAR, WM_KEYDOWN, WM_PAINT...) definidas em windows.h
// ============================================================================


void AddWindows(HWND hWnd) {
	InitBox = CreateWindow(_T("STATIC"),//tipo de janela que se quer
		_T("Jogo dos Tubos"),// Placeholder
		WS_BORDER | WS_CHILD | WS_VISIBLE | SS_CENTER,//carateristicas da janela
		130,//x
		10,//y
		300,//largura
		250,// altura
		hWnd,//Janela
		NULL, NULL, NULL);
	NomeBox = CreateWindow(_T("EDIT"),//tipo de janela que se quer
		_T(""),// Placeholder
		WS_BORDER | WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | SS_CENTER,//carateristicas da janela
		200,//x
		50,//y
		150,//largura
		20,// altura
		hWnd,//Janela
		NULL, NULL, NULL);
	CompetitivoBox = CreateWindow(_T("BUTTON"),//tipo de janela que se quer
		_T("Modo Competitivo"),// Placeholder
		WS_BORDER | WS_CHILD | WS_VISIBLE,//carateristicas da janela
		200,//x
		90,//y
		150,//largura
		50,// altura
		hWnd,//Janela
		(HMENU)MODO_COMPETITIVO, NULL, NULL);
	IndividualBox = CreateWindow(_T("BUTTON"),//tipo de janela que se quer
		_T("Modo Individual"),// Placeholder
		WS_BORDER | WS_CHILD | WS_VISIBLE,//carateristicas da janela
		200,//x
		150,//y
		150,//largura
		50,// altura
		hWnd,//Janela
		(HMENU)MODO_INDIVIDUAL, NULL, NULL);



}
int verificaX(int xPos) {
	int x = xPos - 15;
	return x / 40;
}
int verificaY(int yPos) {
	int y = yPos - 5;
	return y / 40;
}
LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {

	RECT rect;
	static int totalPos = 0;
	PAINTSTRUCT ps;
	static TCHAR c = '_';
	static HANDLE hThread;
	static TCHAR lpvMessage[256] = TEXT("Default message from client.");
	static BOOL   fSuccess = FALSE;
	static DWORD  cbRead, cbToWrite, cbWritten, dwMode;
	static TCHAR nome[20], cbS[20];
	static DWORD modoJogo = ESTADO_INICIAL;
	//static client cliente;
	MINMAXINFO* mmi;

	static int n;
	static HBITMAP hBmp[16];
	static BITMAP bmp;
	static int xBitmap;
	static int yBitmap;
	static int limDir;
	static HANDLE hMutex;
	static int salto;
	static HDC memDC = NULL;
	static int x = 0;
	static int leuPipe = 0;
	switch (messg) {
	case WM_CREATE:
	{

		hdc = GetDC(hWnd);
		bmpDC = CreateCompatibleDC(hdc);
		hBmp[0] = (HBITMAP)LoadImage(NULL, TEXT("17.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[1] = (HBITMAP)LoadImage(NULL, TEXT("5.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[2] = (HBITMAP)LoadImage(NULL, TEXT("2.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[3] = (HBITMAP)LoadImage(NULL, TEXT("1.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[4] = (HBITMAP)LoadImage(NULL, TEXT("6.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[5] = (HBITMAP)LoadImage(NULL, TEXT("3.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[6] = (HBITMAP)LoadImage(NULL, TEXT("4.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[7] = (HBITMAP)LoadImage(NULL, TEXT("14.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[8] = (HBITMAP)LoadImage(NULL, TEXT("7.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[9] = (HBITMAP)LoadImage(NULL, TEXT("8.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[10] = (HBITMAP)LoadImage(NULL, TEXT("10.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[11] = (HBITMAP)LoadImage(NULL, TEXT("9.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[12] = (HBITMAP)LoadImage(NULL, TEXT("12.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[13] = (HBITMAP)LoadImage(NULL, TEXT("15.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);
		hBmp[14] = (HBITMAP)LoadImage(NULL, TEXT("11.bmp"), IMAGE_BITMAP, 40, 40, LR_LOADFROMFILE);


		AddWindows(hWnd);

		break;
	}
	case WM_GETMINMAXINFO:
	{
		mmi = (MINMAXINFO*)lParam;
		WaitForSingleObject(hMutex, INFINITE);
		mmi->ptMinTrackSize.x = bmp.bmWidth + 2;
		mmi->ptMinTrackSize.y = bmp.bmHeight * 2;
		ReleaseMutex(hMutex);
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{

			//InitBox, EditBox, IniciaJogoBox, CompetitivoBox, IndividualBox;
		case MODO_COMPETITIVO: {
			DestroyWindow(InitBox);
			DestroyWindow(NomeBox);
			DestroyWindow(CompetitivoBox);
			DestroyWindow(IndividualBox);
			GetWindowText(NomeBox, dadosCliente.nome, _countof(dadosCliente.nome));
			dadosCliente.modo = MODO_COMPETITIVO;
			dadosCliente.xPos = -1;
			dadosCliente.xPos = -1;

			aux = 1;



			ZeroMemory(&ov, sizeof(ov));
			ov.hEvent = hEvento;

			fSuccess = WriteFile(hPipe, &dadosCliente, sizeof(Client), &n, &ov);
			WaitForSingleObject(ov.hEvent, INFINITE);
			GetOverlappedResult(hPipe, &ov, &n, FALSE);
			if (!fSuccess)
			{
				//  MessageBox(hWnd, TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError(), _T("Erro"), MB_OK);
				return -1;
			}
			ZeroMemory(&ov, sizeof(ov));
			ov.hEvent = hEvento;
			SetEvent(dados.hEvent[HEVENT_ESCREVE_CLIENTE]);
			ResetEvent(dados.hEvent[HEVENT_ESCREVE_CLIENTE]);
			fSuccess = ReadFile(
				hPipe,    // pipe handle 
				&dadosCliente,    // buffer to receive reply 
				sizeof(Client),  // size of buffer 
				&n,  // number of bytes read 
				&overlap);    //  overlapped 

			WaitForSingleObject(ov.hEvent, INFINITE);
			GetOverlappedResult(hPipe, &ov, &n, FALSE);
			if (!fSuccess)
			{
				//  MessageBox(hWnd, TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError(), _T("Erro"), MB_OK);
				return -1;
			}


			break;
		}
		case MODO_INDIVIDUAL: {

			GetWindowText(NomeBox, dadosCliente.nome, _countof(nome));

			dadosCliente.modo = MODO_INDIVIDUAL;
			aux = 1;
			DestroyWindow(InitBox);
			DestroyWindow(NomeBox);
			DestroyWindow(CompetitivoBox);
			DestroyWindow(IndividualBox);

			dadosCliente.xPos = -1;
			dadosCliente.xPos = -1;

			aux = 1;



			ZeroMemory(&ov, sizeof(ov));
			ov.hEvent = hEvento;

			fSuccess = WriteFile(hPipe, &dadosCliente, sizeof(Client), &n, &ov);
			WaitForSingleObject(ov.hEvent, INFINITE);
			GetOverlappedResult(hPipe, &ov, &n, FALSE);
			if (!fSuccess)
			{
				//  MessageBox(hWnd, TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError(), _T("Erro"), MB_OK);
				return -1;
			}
			ZeroMemory(&ov, sizeof(ov));
			ov.hEvent = hEvento;
			SetEvent(dados.hEvent[HEVENT_ESCREVE_CLIENTE]);
			fSuccess = ReadFile(
				hPipe,    // pipe handle 
				&dadosCliente,    // buffer to receive reply 
				sizeof(Client),  // size of buffer 
				&n,  // number of bytes read 
				&overlap);    //  overlapped 

			WaitForSingleObject(ov.hEvent, INFINITE);
			GetOverlappedResult(hPipe, &ov, &n, FALSE);
			if (!fSuccess)
			{
				//  MessageBox(hWnd, TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError(), _T("Erro"), MB_OK);
				return -1;
			}



			break;
		}

		}
		break;
	}
	case WM_LBUTTONDOWN: // <-BOTAO ESQUERDO, BOTADO DIREITO -> WM_RBUTTONDOWN
	{
		if (dadosCliente.modo == ESTADO_INICIAL) {
			break;
		}

		//MessageBox(hWnd, _T("Sair"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
		dadosCliente.xPos = verificaX(GET_X_LPARAM(lParam));
		dadosCliente.yPos = verificaY(GET_Y_LPARAM(lParam));



		if (dadosCliente.xPos >= dadosCliente.mapa.mapSize || dadosCliente.yPos >= dadosCliente.mapa.mapSize || (dadosCliente.xPos == dadosCliente.mapa.origemX && dadosCliente.yPos == dadosCliente.mapa.origemY)
			|| (dadosCliente.xPos == dadosCliente.mapa.destinoX && dadosCliente.yPos == dadosCliente.mapa.destinoY)) {
			//MessageBox(hWnd, _T("NAO DA"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
			break;
		}

		_tcscpy_s(dadosCliente.nome, 100, _T("joga"));
		ZeroMemory(&ov, sizeof(ov));
		ov.hEvent = hEvento;
		fSuccess = WriteFile(hPipe, &dadosCliente, sizeof(Client), &n, &ov);
		if (!fSuccess && GetLastError() == ERROR_IO_PENDING) {
			//MessageBox(hWnd, _T("OuveServidor2"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);
			WaitForSingleObject(ov.hEvent, INFINITE);
			GetOverlappedResult(hPipe, &overlap, &n, FALSE);
		}
		_tcscpy_s(dadosCliente.nome, 100, _T(""));


		InvalidateRect(hWnd, NULL, TRUE); // requisita WM_PAINT

		break;
	}

	case WM_RBUTTONDOWN:
	{
		if (dadosCliente.modo == ESTADO_INICIAL) {
			break;
		}

		//InvalidateRect(td.hWnd, NULL, TRUE); // requisita WM_PAINT
		break;
	}

	case WM_CLOSE:
		if (MessageBox(hWnd, _T("Queres sair da janela?"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP) == IDYES)
		{

			CloseHandle(hPipe);
			DestroyWindow(hWnd);
		}
		break;
	case WM_PAINT:
	{

		if (aux != 1) {
			break;
		}
		hdc = BeginPaint(hWnd, &ps);

		int i = 0, j;
		int Lx1 = 5, Lx2 = 45;
		int Cy1 = 30, Cy2 = 70;

		BOOL existe = FALSE;
		static int count = 0;

		for (int i = 0; i < dadosCliente.mapa.mapSize; i++)
		{
			for (int j = 0; j < dadosCliente.mapa.mapSize; j++)
			{


				if (dadosCliente.mapa.arrMapa[i][j] == 120) { //x

					GetObject(hBmp[0], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[0]);

					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9472) { //'─'
					if (i == dadosCliente.mapa.origemX && j == dadosCliente.mapa.origemY) {

						GetObject(hBmp[7], sizeof(bmp), &bmp);
						SelectObject(bmpDC, hBmp[7]);
						existe = TRUE;
					}
					else if (i == dadosCliente.mapa.destinoX && j == dadosCliente.mapa.destinoY) {

						GetObject(hBmp[13], sizeof(bmp), &bmp);
						SelectObject(bmpDC, hBmp[13]);
						existe = TRUE;
					}
					else {
						//MessageBox(td.hWnd, _T("8"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);

						GetObject(hBmp[14], sizeof(bmp), &bmp);
						SelectObject(bmpDC, hBmp[14]);
						existe = TRUE;
					}

				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9552) {
					//MessageBox(td.hWnd, _T("2"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);

					GetObject(hBmp[1], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[1]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9556) {//_T('╔')
					//MessageBox(td.hWnd, _T("3"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);

					GetObject(hBmp[2], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[2]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9559) {//_T('╗')
					//MessageBox(td.hWnd, _T("4"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);

					GetObject(hBmp[3], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[3]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9553) {//_T('║')
					//MessageBox(td.hWnd, _T("5"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);

					GetObject(hBmp[4], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[4]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9565) {//_T('╝')
					//MessageBox(td.hWnd, _T("6"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);

					GetObject(hBmp[5], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[5]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9562) {// _T('╚')
					//MessageBox(td.hWnd, _T("7"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);

					GetObject(hBmp[6], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[6]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9488) {//_T('┐')
					//MessageBox(td.hWnd, _T("9"), _T("Sair"), MB_ICONQUESTION | MB_YESNO | MB_HELP);

					GetObject(hBmp[8], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[8]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9484) {//_T('┌')

					GetObject(hBmp[9], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[9]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9492) {//_T('└')

					GetObject(hBmp[10], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[10]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 9496) {// _T('┘')

					GetObject(hBmp[11], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[11]);
					existe = TRUE;
				}
				else if (dadosCliente.mapa.arrMapa[i][j] == 124) {//_T('|')

					GetObject(hBmp[12], sizeof(bmp), &bmp);
					SelectObject(bmpDC, hBmp[12]);
					existe = TRUE;
				}

				if (existe) {

					int x = i * 40 + 5;
					int y = j * 40 + 15;
					BitBlt(hdc, y,// vai direita
						x,// vai baixo
						bmp.bmWidth, bmp.bmHeight, bmpDC,
						0,// vai para a esquerda
						0,// vai para cima
						SRCCOPY);
				}
			}

		}

		EndPaint(hWnd, &ps);

		break;
	}

	case WM_CHAR:
		c = (TCHAR)wParam;
		break;

	case WM_HELP:
		MessageBox(hWnd, _T("janela de ajuda"), _T("Ajuda"), MB_OK);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:

		return(DefWindowProc(hWnd, messg, wParam, lParam));
		break;

		return(0);
	}
}