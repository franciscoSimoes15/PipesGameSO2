#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <Windows.h>
#include <conio.h>
#include "..//Servidor//comunicacao.h"

#define BUFSIZE 24

int _tmain(int argc, TCHAR* argv[])
{
    HANDLE hPipe;
    LPTSTR lpvMessage = TEXT("Default message from client.");
    TCHAR  chBuf[BUFSIZE];
    BOOL   fSuccess = FALSE;
    DWORD  cbRead, cbToWrite, cbWritten, dwMode;

    if (argc > 1)
        lpvMessage = argv[1];

    // Try to open a named pipe; wait for it, if necessary. 

    _tprintf(TEXT("[LEITOR] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), PIPE_SERVER);

    //espera que exista um named pipe para ler do mesmo
    //bloqueia aqui
    if (!WaitNamedPipe(PIPE_SERVER, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_SERVER);
        exit(-1);
    }

    _tprintf(TEXT("[LEITOR] Ligação ao pipe do escritor... (CreateFile)\n"));

    //ligamo-nos ao named pipe que ja existe nesta altura
    //1º nome do named pipe, 2ºpermissoes (têm de ser iguais ao CreateNamedPipe do servidor), 3ºshared mode 0 aqui,
    //4º security atributes, 5ºflags de criação OPEN_EXISTING, 6º o default é FILE_ATTRIBUTE_NORMAL e o 7º é o template é NULL
    hPipe = CreateFile(PIPE_SERVER, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_SERVER);
        exit(-1);
    }

    _tprintf(TEXT("[LEITOR] Liguei-me...\n"));

        // All pipe instances are busy, so wait for 20 seconds. 

        
  

    // The pipe connected; change to message-read mode. 
    while(1){
    dwMode = PIPE_READMODE_MESSAGE;
    fSuccess = SetNamedPipeHandleState(
        hPipe,    // pipe handle 
        &dwMode,  // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL);    // don't set maximum time 
    if (!fSuccess)
    {
        _tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
        return -1;
    }

    // Send a message to the pipe server. 

    //cbToWrite = (lstrlen(lpvMessage) + 1) * sizeof(TCHAR);
    //_tprintf(TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, lpvMessage);
    
    _tprintf(TEXT("[Leitor] Frase: "));
    _fgetts(chBuf, 256, stdin);
    chBuf[_tcslen(chBuf) - 1] = '\0';
    cbToWrite = (_tcslen(chBuf) + 1) * sizeof(TCHAR);
    _tprintf(TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, chBuf);

    fSuccess = WriteFile(
        hPipe,                  // pipe handle 
        chBuf,             // message lpvMessage
        cbToWrite,              // message length 
        &cbWritten,             // bytes written 
        NULL);                  // not overlapped 

    if (!fSuccess)
    {
        _tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
        return -1;
    }
    chBuf[cbToWrite / sizeof(TCHAR)] = '\0';
    printf("\nMessage sent to server, receiving reply as follows:\n");

        do
        {
            // Read from the pipe. 

            fSuccess = ReadFile(
                hPipe,    // pipe handle 
                chBuf,    // buffer to receive reply 
                sizeof(chBuf),  // size of buffer 
                &cbRead,  // number of bytes read 
                NULL);    // not overlapped 

            if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
                break;
            //termina corretamente a string
            chBuf[cbRead / sizeof(TCHAR)] = '\0';

            _tprintf(TEXT("\"%s\"\n"), chBuf);  
        } while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

        if (!fSuccess)
        {
            _tprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
            return -1;
        }
    
    }

    printf("\n<End of message, press ENTER to terminate connection and exit>");
    _getch();

    CloseHandle(hPipe);

    return 0;
}