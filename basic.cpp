/********************************************************************************
*    Programa    : basic.c
*    Objetivo    : MMSJ-Basic para o MMSJ800
*    Criado em   : 12/02/2025
*    Programador : Moacir Jr.
*--------------------------------------------------------------------------------
* Data        Versao  Responsavel  Motivo
* 10/10/2022  0.9     Moacir Jr.   Conversão do MMSJ300 68000 para o MMSJ800 STM32
*--------------------------------------------------------------------------------
* Variables Simples: start at 00800000
*   --------------------------------------------------------
*   Type ($ = String, # = Real, % = Integer)
*   Name (2 Bytes, 1st and 2nd letters of the name)
*   --------------- --------------- ------------------------
*   Integer         Real            String
*   --------------- --------------- ------------------------
*   0x00            0x00            Length
*   Value MSB       Value MSB       Pointer to String (High)
*   Value           Value           Pointer to String
*   Value           Value           Pointer to String
*   Value LSB       Value LSB       Pointer to String (Low)
*   --------------- --------------- ------------------------
*   Total: 8 Bytes
*--------------------------------------------------------------------------------
*
*--------------------------------------------------------------------------------
* To do
*
*--------------------------------------------------------------------------------
*
*********************************************************************************/
//#define __TESTE_TOKENIZE__ 1
//#define __DEBUG_ARRAYS__ 1

// Apenas até tudo ter sido convertido, o que nao estiver ainda, ficara desabilitado por esse define
//#define __NOT_USING_ON_BASIC__ 1

// Esse define é pq o rand esta crashando o MCU, mesmo com o RNG ligado. Usando customizado
//#define __USE_CPP_RAND_FUNC__ 1

#define __USE_ACOMP_TOK__ 1

#include "UartRingbuffer_multi.h"
#include "ctype.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "fatfs.h"
#include "basic.h"
#include "basicPeriph.h"

#define versionBasic "0.9"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

extern TIM_HandleTypeDef htim11;
extern TIM_HandleTypeDef htim13;

#define wifi_uart &huart1
#define pc_uart &huart2

unsigned char *pStartSimpVar  = (unsigned char*)malloc(BASIC_SIZE_VAR);         // Area Variaveis Simples
unsigned char *pStartArrayVar = (unsigned char*)malloc(BASIC_SIZE_ARRAY_VAR);   // Area Arrays
unsigned char *pStartString   = (unsigned char*)malloc(BASIC_SIZE_STRING);      // Area Strings
unsigned char *pStartProg     = (unsigned char*)malloc(BASIC_SIZE_PROG);        // Area Programa  deve ser 0x00810000

unsigned char *token = (unsigned char*)malloc(256);
unsigned long *vDataBkpPointerProg = (unsigned long*)malloc(4);
unsigned long *vDataPointer = (unsigned long*)malloc(4);

extern uint16_t scrollbuf[480];
extern void windowScroll(int16_t x, int16_t y, int16_t wid, int16_t ht, int16_t dx, int16_t dy, uint16_t *buf);

//-----------------------------------------------------------------------------
void writech(char c)
{
    outch(c);
}

//-----------------------------------------------------------------------------
void print(char* s)
{
    outs(s);
    if (outGetPosY() > out_height && !outGetRotation() && vdp_mode == TFT_MODE_TEXT)
    {
        windowScroll(0, 0, out_width, out_height, 0, 15, scrollbuf);
        outFillRect(0, out_height - TFT_FATOR_ROW, out_width, TFT_FATOR_ROW, TFT_BLACK);
        outSetCursor(0,out_height - 1);
    }    
}

//-----------------------------------------------------------------------------
void println(char* s)
{
    outsln(s);
    if (outGetPosY() > out_height && !outGetRotation() && vdp_mode == TFT_MODE_TEXT)
    {
        windowScroll(0, 0, out_width, out_height, 0, 15, scrollbuf);
        outFillRect(0, out_height - TFT_FATOR_ROW, out_width, TFT_FATOR_ROW, TFT_BLACK);
        outSetCursor(0,out_height - 1);
    }
}

//-----------------------------------------------------------------------------
char readChar(char pTipo)
{
    char c;

    c = inch(pTipo);

    return c;
}


//-----------------------------------------------------------------------------
void hideCursor(void)
{
    writech(0x00);
}

//-----------------------------------------------------------------------------
void showCursor(void)
{
    writech('_' /*0xFF*/ );
}

//-----------------------------------------------------------------------------
// pQtdInput - Quantidade a ser digitada, min 1 max 255
// pTipo - Tipo de entrada:
//                  input : $ - String, % - Inteiro (sem ponto), # - Real (com ponto), @ - Sem Cursor e Qualquer Coisa e sem enter
//                   edit : S - String, I - Inteiro (sem ponto), R - Real (com ponto)
//-----------------------------------------------------------------------------
char readStr(unsigned int pQtdInput, unsigned char pTipo, char* vbuf)
{
    char *vbufptr = vbuf;
    unsigned char vtec, vtecant;
    int countCursor = 0;

    if (pQtdInput == 0)
        pQtdInput = 512;

    vtecant = 0x00;

    if (pTipo != '@')
        showCursor();

    while (1)
    {
        // Inicia leitura
        vtec = readChar(KEY_WAIT);

        if (pTipo == '@')
            return vtec;

        // Se nao for string ($ e S) ou Tudo (@), só aceita numeros
        if (pTipo != '$' && pTipo != 'S' && pTipo != '@' && vtec != '.' && vtec > 0x1F && (vtec < 0x30 || vtec > 0x39))
            vtec = 0;

        // So aceita ponto de for numero real (# ou R) ou string ($ ou S) ou tudo (@)
        if (vtec == '.' && pTipo != '#' && pTipo != '$' &&  pTipo != 'R' && pTipo != 'S' && pTipo != '@')
            vtec = 0;

        if (vtec)
        {
            // Prevenir sujeira no buffer ou repeticao
            if (vtec == vtecant)
            {
                if (countCursor % 300 != 0)
                    continue;
            }

            /*if (pTipo != '@')
            {
                hideCursor();
            }*/

            vtecant = vtec;

            if (vtec >= 0x20 && vtec != 0x7F)   // Caracter Printavel menos o DELete
            {
                // Digitcao Normal
                if (vbufptr > vbuf + pQtdInput)
                {
                    vbufptr--;

                    if (pTipo != '@')
                    	writech(0x08);
                }

                if (pTipo != '@')
                	writech(vtec);

                *vbufptr++ = vtec;
                *vbufptr = '\0';
            }
            else if (vtec == 0x08)  // Backspace
            {
                // Digitcao Normal
                if (vbufptr > vbuf)
                {
                    vbufptr--;
                    *vbufptr = 0x00;

                    if (pTipo != '@')
                        writech(0x08);
                }
            }
            else if (vtec == 0x1B)   // ESC
            {
                // Limpa a linha, esvazia o buffer e retorna tecla
                while (vbufptr > vbuf)
                {
                    vbufptr--;
                    *vbufptr = 0x00;

                    /*if (pTipo != '@')
                        hideCursor();*/

                    if (pTipo != '@')
                        writech(0x08);

                    /*if (pTipo != '@')
                        showCursor();*/
                }
                //hideCursor();

                return vtec;
            }
            else if (vtec == 0x0D || vtec == 0x0A ) // CR ou LF
            {
                return vtec;
            }

            /*if (pTipo != '@')
                showCursor();*/
        }
        else
        {
            vtecant = 0x00;
        }
    }

    return 0x00;
}

//-----------------------------------------------------------------------------
#ifdef IN_TOUCH
void waitTouch(void)
{
    TSData tsPos;
    
    outSetCursor((out_width - (21 * TFT_FATOR_COL)), out_height - 1);
    print((char*)"Touch to continue...");
    while(1)
    {
        // Wait touch do continue
        pKbdData.pkeyativo = 0;
        TCHVerif(&tsPos, 0, &pKbdData);

        if (tsPos.x != 0 || tsPos.y != 0)
            break;
    }
}
#endif

//-----------------------------------------------------------------------------
// Limpa Tela
//-----------------------------------------------------------------------------
void clearScr(void)
{
    outFillScr(TFT_BLACK);
    outSetCursor(0,TFT_FATOR_ROW - 1);
}

//-----------------------------------------------------------------------------
// Principal
//-----------------------------------------------------------------------------
#ifdef __IMPORT_FILE_BAS__
#ifdef __MAIN_PARAM__
int main(int argc, char *argv[])
#endif
#ifdef __FUNC_TO_CALL__
int execBasic(char* filename)
#endif
{
    char data[80], cbas[2];
    unsigned char tokenData[256];
    int icount, ilin, iBytes = 0, iBarras = 0;
    int16_t ipx, ipy;
    DWORD iFator, iSize;
    FRESULT res;
    FIL basicFile;
    UINT br;
    char numLinha[6];
    unsigned int fLine, lLine, tLine;

    #ifdef __MAIN_PARAM__
    char filename[50];
    memcpy(filename, argv[1])
    #endif

    vErroProc = 0;

    if (!strstr(filename,".bas"))
        return 0xFF03;

    // Timer para o Random
    vdp_mode = TFT_MODE_TEXT;
    out_width = outGetWidth();
    out_height = outGetHeight();
    out_heighttotal = outGetHeight();

    TFT_FATOR_COL = 12;
    TFT_FATOR_ROW = 16;

    outFillScr(TFT_BLACK);
    outSetTextColor(TFT_WHITE, TFT_BLACK);
    outSetCursor(0, TFT_FATOR_ROW - 1);
    println((char*)"MMSJ-BASIC v" versionBasic);
    println((char*)"Utility (c) 2022-2025");
    println((char*)"");

    println((char*)"--- Memory Spaces --- ");
    print((char*)"  Variables..: 0x");
    sprintf(data,"%lX = 4KB", (unsigned long)pStartSimpVar);
    println(data);
    print((char*)"  Arrays.....: 0x");
    sprintf(data,"%lX = 8KB", (unsigned long)pStartArrayVar);
    println(data);
    print((char*)"  Strings....: 0x");
    sprintf(data,"%lX = 8KB", (unsigned long)pStartString);
    println(data);
    print((char*)"  Program....: 0x");
    sprintf(data,"%lX = 16KB", (unsigned long)pStartProg);
    println(data);
    println((char*)"");

    if ((unsigned long)pStartSimpVar == 0 || 
        (unsigned long)pStartArrayVar == 0 || 
        (unsigned long)pStartString == 0 ||
        (unsigned long)pStartProg == 0)
    {
        println((char*)"Error: Not enough memory to run");        
    
        #ifdef IN_TOUCH
        waitTouch();
        #endif

        /*free(pStartProg);
        free(pStartString);
        free(pStartArrayVar);
        free(pStartSimpVar);*/
        
        return 0xFF01;
    }

    sprintf(data,"Tokenizing file %s...", filename);
    println(data);        

    #ifdef __USE_ACOMP_TOK__    
        println((char*)"   1  2  3  4  5  6  7  8  9   ");
        println((char*)"0..0..0..0..0..0..0..0..0..0..F");        
        print  ((char*)"   |  |  |  |  |  |  |  |  |  |"); 
        ipy = outGetPosY();
        println((char*)" ");
        println((char*)"Processing Line: 0");
        outSetCursor(0, ipy);
    #endif

    pProcess = 0x01;
    pTypeLine = 0x00;
    nextAddrLine = (unsigned long)pStartProg;
    firstLineNumber = 0;
    addrFirstLineNumber = 0;
    traceOn = 0;
    lastHgrX = 0;
    lastHgrY = 0;
    vErroProc = 0;

    // Read File and *tokenize line by line
    res = f_open(&basicFile, filename, FA_READ);
    if (res != FR_OK)
        return(res);

    iSize = f_size(&basicFile);
    iFator = iSize / 30;
    icount = 0;
    ilin = 0;
    fLine = 0;
    lLine = 0;
    tLine = 0;

    while(1)
    {
        if (f_read(&basicFile, &cbas, 1, &br) == FR_OK)
        {
            iBytes++;

            if (cbas[0] != '\0')
            {
                if (!icount)
                {
                    ilin = 0; 
                    while (cbas[0] != ' ')
                    {
                        numLinha[ilin++] = cbas[0];

                        if (f_read(&basicFile, &cbas, 1, &br) != FR_OK)
                            break;
        
                        iBytes++;
                    }
                    numLinha[ilin++] = '\0';

                    if (f_read(&basicFile, &cbas, 1, &br) != FR_OK)
                        break;
        
                    iBytes++;

                    #ifdef __USE_ACOMP_TOK__    
                        ipx = outGetPosX();
                        outSetCursor(0, (ipy + TFT_FATOR_ROW));
                        sprintf(data, "Processing Line: %d", atoi(numLinha));
                        print(data);
                        outSetCursor(ipx, ipy);
                    #endif

                    if (!fLine)
                        fLine = atoi(numLinha);
                }
                
                // Save char
                if (cbas[0] != '\n')
                {
                    if (cbas[0] < 0x20)
                    {
                        tokenData[icount++] = (unsigned char)'\0';
                    }
                    else
                    {
                        tokenData[icount++] = (unsigned char)cbas[0];
                    }                        
                }
                else  // If char = LF, send to *tokenize and save line
                {
                    tokenizeLine(tokenData);
                    lLine = atoi(numLinha);
                    if (saveLine(numLinha, tokenData))  // qualquer coisa <> 0 é erro
                    {
                        vErroProc = 0xFF02;
                        break;
                    }
                    icount = 0;
                    tLine++;
                }
            }

            #ifdef __USE_ACOMP_TOK__    
                if (iBarras <= 30 && (iBytes % iFator) == 0)
                {
                    iBarras++;
                    print((char*)"#");
                }
            #endif
        }
        else
            break;
    }

    f_close(&basicFile);

    if (!vErroProc)
    {
        #ifdef __USE_ACOMP_TOK__    
            for (icount = iBarras; icount <= 30; icount++)
                print((char*)"#");

            outSetCursor(0, (ipy + TFT_FATOR_ROW));
            println((char*)"Processing Line: Done!       ");
        #endif

        println((char*)"--- Resume --- ");
        print((char*)"  First Line..: ");
        sprintf(data,"%d", fLine);
        println(data);
        print((char*)"  Last Line...: ");
        sprintf(data,"%d", lLine);
        println(data);
        print((char*)"  Total Lines.: ");
        sprintf(data,"%d", tLine);
        println(data);
        println((char*)"");

        #ifdef IN_TOUCH
        waitTouch();

        outSetRotation(0);
        outSetFont(&DialogInput_plain_12); 
        TFT_FATOR_COL = 8;
        TFT_FATOR_ROW = 15;       
        #endif

        out_width = outGetWidth();
        out_height = outGetHeight();
        out_heighttotal = outGetHeight();

        #ifdef IN_TOUCH
        out_height -= 180;  // Retira a area do teclado virtual (115) e do status (65)

        // Prepara area do teclado
        outFillRect(0, out_height + 2, out_width, 25, TFT_BLACK);
        outDrawRect(0, out_height + 2, out_width, 25, TFT_WHITE);
        outFillRect(0, out_height + 28, out_width, 150, TFT_BLACK);
        outDrawRect(0, out_height + 28, out_width, 150, TFT_WHITE);
        
        // Abre teclado permanente
        pKbdData.keyvxx = 5;
        pKbdData.keyvyy = out_height + 31;
        pKbdData.vtipokey = 0;      // ABC
        pKbdData.vtipofimkey = 1;   // Return
        pKbdData.pShow = 1;
        pKbdData.vkeyrep = 0;
        pKbdData.vcapson = 0;
        pKbdData.pkeyativo = 1;

        ShowKeyboard(&pKbdData, 0);
        #endif

        runProg((unsigned char*)"\0");

        #ifdef IN_TOUCH
        outSetRotation(1);
        outSetFont(&FreeMono9pt7b);        
        #endif
    }
    else
    {
        outSetCursor(0, (ipy + TFT_FATOR_ROW));
        println((char*)"Processing Line: ERROR!      ");

        #ifdef IN_TOUCH
        waitTouch();
        #endif
    }   

    /*free(pStartProg);
    free(pStartString);
    free(pStartArrayVar);
    free(pStartSimpVar);
    free(token);
    free(vDataBkpPointerProg);
    free(vDataPointer);*/

    return 0;
}
#endif

#ifdef __COMMAND_LINE__
//-----------------------------------------------------------------------------
void main(void)
{
    unsigned char vRetInput;

    println((char*)"MMSJ-BASIC v" versionBasic);
    println((char*)"Utility (c) 2022-2025");
    println((char*)"OK");

    *vBufReceived = 0x00;
    *vbuf = '\0';
    *pProcess = 0x01;
    *pTypeLine = 0x00;
    *nextAddrLine = pStartProg;
    *firstLineNumber = 0;
    *addrFirstLineNumber = 0;
    *traceOn = 0;
    *lastHgrX = 0;
    *lastHgrY = 0;
    *fgcolorAnt = *fgcolor;
    *bgcolorAnt = *bgcolor;

    while (*pProcess)
    {
        vRetInput = inputLine(128,'$');

        if (*vbuf != 0x00 && (vRetInput == 0x0D || vRetInput == 0x0A))
        {
            printText("\r\n\0");

            processLine();

            if (!*pTypeLine && *pProcess)
                printText("\r\nOK\0");

            *vBufReceived = 0x00;
            *vbuf = '\0';

            if (!*pTypeLine && *pProcess)
                printText("\r\n\0");   // printText("\r\n>\0");
        }
        else if (vRetInput != 0x1B)
        {
            printText("\r\n\0");
        }
    }

    printText("\r\n\0");
}

//-----------------------------------------------------------------------------
void processLine(void)
{
    unsigned char linhacomando[32], vloop, vToken;
    unsigned char *blin = vbuf;
    unsigned short varg = 0;
    unsigned short ix, iy, iz, ikk, kt;
    unsigned short vbytepic = 0, vrecfim;
    unsigned char cuntam, vLinhaArg[255], vparam2[16], vpicret;
    char vSpace = 0;
    int vReta;
    typeInf vRetInf;
    unsigned short vTam = 0;
    unsigned char *pSave = *nextAddrLine;
    unsigned long vNextAddr = 0;
    unsigned char vTimer;
    unsigned char vBuffer[20];
    unsigned char *vTempPointer;
    unsigned char sqtdtam[20];

    // Separar linha entre comando e argumento
    linhacomando[0] = '\0';
    vLinhaArg[0] = '\0';
    ix = 0;
    iy = 0;
    while (*blin)
    {
        if (!varg && *blin >= 0x20 && *blin <= 0x2F)
        {
            varg = 0x01;
            linhacomando[ix] = '\0';
            iy = ix;
            ix = 0;

            if (*blin != 0x20)
                vLinhaArg[ix++] = *blin;
            else
                vSpace = 1;
        }
        else
        {
            if (!varg)
                linhacomando[ix] = *blin;
            else
                vLinhaArg[ix] = *blin;
            ix++;
        }

        *blin++;
    }

    if (!varg)
    {
        linhacomando[ix] = '\0';
        iy = ix;
    }
    else
        vLinhaArg[ix] = '\0';

    vpicret = 0;

    // Processar e definir o que fazer
    if (linhacomando[0] != 0)
    {
        // Se for numero o inicio da linha, eh entrada de programa, senao eh comando direto
        if (linhacomando[0] >= 0x31 && linhacomando[0] <= 0x39) // 0 nao é um numero de linha valida
        {
            *pTypeLine = 0x01;

            // Entrada de programa
            tokenizeLine(vLinhaArg);
            saveLine(linhacomando, vLinhaArg);
        }
        else
        {
            *pTypeLine = 0x00;

            for (iz = 0; iz < iy; iz++)
                linhacomando[iz] = toupper(linhacomando[iz]);

            // Comando Direto
            if (!strcmp(linhacomando,"HOME") && iy == 4)
            {
                clearScr();
            }
            else if (!strcmp(linhacomando,"NEW") && iy == 3)
            {
                *pStartProg = 0x00;
                *(pStartProg + 1) = 0x00;
                *(pStartProg + 2) = 0x00;

                *nextAddrLine = pStartProg;
                *firstLineNumber = 0;
                *addrFirstLineNumber = 0;
            }
            else if (!strcmp(linhacomando,"EDIT") && iy == 4)
            {
                editLine(vLinhaArg);
            }
            else if (!strcmp(linhacomando,"LIST") && iy == 4)
            {
                listProg(vLinhaArg, 0);
            }
            else if (!strcmp(linhacomando,"LISTP") && iy == 5)
            {
                listProg(vLinhaArg, 1);
            }
            else if (!strcmp(linhacomando,"RUN") && iy == 3)
            {
                runProg(vLinhaArg);
            }
            else if (!strcmp(linhacomando,"DEL") && iy == 3)
            {
                delLine(vLinhaArg);
            }
            else if (!strcmp(linhacomando,"XLOAD") && iy == 5)
            {
                basXBasLoad();
            }
            else if (!strcmp(linhacomando,"TIMER") && iy == 5)
            {
                // Ler contador A do 68901
                vTimer = *(vmfp + Reg_TADR);

                // Devolve pra tela
                itoa(vTimer,vBuffer,10);
                printText("Timer: ");
                printText(vBuffer);
                printText("ms\r\n\0");
            }
            else if (!strcmp(linhacomando,"TRACE") && iy == 5)
            {
                *traceOn = 1;
            }
            else if (!strcmp(linhacomando,"NOTRACE") && iy == 7)
            {
                *traceOn = 0;
            }
            // *************************************************
            // ESSE COMANDO NAO VAI EXISTIR QUANDO FOR PRA BIOS
            // *************************************************
            else if (!strcmp(linhacomando,"QUIT") && iy == 4)
            {
                *pProcess = 0x00;
            }
            // *************************************************
            // *************************************************
            // *************************************************
            else
            {
                // Tokeniza a linha toda
                strcpy(vRetInf.tString, linhacomando);

                if (vSpace)
                    strcat(vRetInf.tString, " ");

                strcat(vRetInf.tString, vLinhaArg);

                tokenizeLine(vRetInf.tString);

                strcpy(vLinhaArg, vRetInf.tString);

                // Salva a linha pra ser interpretada
                vTam = strlen(vLinhaArg);
                vNextAddr = comandLineTokenized + (vTam + 6);

                *comandLineTokenized = ((vNextAddr & 0xFF0000) >> 16);
                *(comandLineTokenized + 1) = ((vNextAddr & 0xFF00) >> 8);
                *(comandLineTokenized + 2) =  (vNextAddr & 0xFF);

                // Grava numero da linha
                *(comandLineTokenized + 3) = 0xFF;
                *(comandLineTokenized + 4) = 0xFF;

                // Grava linha tokenizada
                for(kt = 0; kt < vTam; kt++)
                    *(comandLineTokenized + (kt + 5)) = vLinhaArg[kt];

                // Grava final linha 0x00
                *(comandLineTokenized + (vTam + 5)) = 0x00;
                *(comandLineTokenized + (vTam + 6)) = 0x00;
                *(comandLineTokenized + (vTam + 7)) = 0x00;
                *(comandLineTokenized + (vTam + 8)) = 0x00;

                *nextAddrSimpVar = pStartSimpVar;
                *nextAddrArrayVar = pStartArrayVar;
                *nextAddrString = pStartString;
                *vMaisTokens = 0;
                *vParenteses = 0x00;
                *vTemIf = 0x00;
                *vTemThen = 0;
                *vTemElse = 0;
                *vTemIfAndOr = 0x00;

                *pointerRunProg = comandLineTokenized + 5;

                vRetInf.tString[0] = 0x00;
                *ftos=0;
                *gtos=0;
                *vErroProc = 0;
                *randSeed = *(vmfp + Reg_TADR);
                do
                {
                    *doisPontos = 0;
                    *vInicioSentenca = 1;
                    vTempPointer = *pointerRunProg;
                    *pointerRunProg = *pointerRunProg + 1;
                    vReta = executeToken(*vTempPointer);
                } while (*doisPontos);

#ifndef __TESTE_TOKENIZE__
                if (*vdp_mode != VDP_MODE_TEXT)
                    basText();
#endif
                if (*vErroProc)
                {
                    showErrorMessage(*vErroProc, 0);
                }
            }
        }
    }
}
#endif

//-----------------------------------------------------------------------------
// Transforma linha em *tokens, se existirem
//-----------------------------------------------------------------------------
void tokenizeLine(unsigned char *pTokenized)
{
    char vLido[255], vLidoCaps[255], vAspas;
    unsigned char *blin = pTokenized;
    unsigned short ix, iy, kt, iz, iw;
    unsigned char vToken, vLinhaArg[255];
    char vFirstComp = 0;
    char vTemRem = 0;

    // Separar linha entre comando e argumento
    vLinhaArg[0] = '\0';
    vLido[0]  = '\0';
    ix = 0;
    iy = 0;
    vAspas = 0;

    while (1)
    {
        vLido[ix] = '\0';

        if (*blin == 0x22)
            vAspas = !vAspas;

        // Se for quebrador sequencia, verifica se é um token
        if ((!vTemRem && !vAspas && strchr(" ;,+-<>()/*^=:",*blin)) || !*blin)
        {
            // Montar comparacoes "<>", ">=" e "<="
            if (((*blin == 0x3C || *blin == 0x3E) && (!vFirstComp && (*(blin + 1) == 0x3E || *(blin + 1) == 0x3D))) || (vFirstComp && *blin == 0x3D) || (vFirstComp && *blin == 0x3E))
            {
                if (!vFirstComp)
                {
                    for(kt = 0; kt < ix; kt++)
                        vLinhaArg[iy++] = vLido[kt];
                    vLido[0] = 0x00;
                    ix = 0;
                    vFirstComp = 1;
                }

                vLido[ix++] = *blin;

                if (ix < 2)
                {
                    blin++;

                    continue;
                }

                vFirstComp = 0;
            }

            if (vLido[0])
            {
                vToken = 0;

                if (ix > 1)
                {
                    // Transforma em Caps pra comparar com os *tokens
                    for (kt = 0; kt < ix; kt++)
                        vLidoCaps[kt] = toupper(vLido[kt]);

                    vLidoCaps[ix] = 0x00;

                    iz = strlen(vLidoCaps);

					// Compara pra ver se é um token
					for(kt = 0; kt < keywords_count; kt++)
					{
						iw = strlen(keywords[kt].keyword);

                        if (iw == 2 && iz == iw)
                        {
                            if (vLidoCaps[0] == keywords[kt].keyword[0] && vLidoCaps[1] == keywords[kt].keyword[1])
                            {
                                vToken = keywords[kt].token;
                                break;
                            }
                        }
                        else if (iz==iw)
                        {
                            if (strncmp(vLidoCaps, keywords[kt].keyword, iw) == 0)
                            {
                                vToken = keywords[kt].token;
                                break;
                            }
                        }
					}
                }

                if (vToken)
                {
                    if (vToken == 0x8C) // REM
                        vTemRem = 1;

                    vLinhaArg[iy++] = vToken;

                    //if (*blin == 0x28 || *blin == 0x29)
                    //    vLinhaArg[iy++] = *blin;

                    //if (*blin == 0x3A)  // :
                    if (*blin && *blin != 0x20 && vToken < 0xF0 && !vTemRem)
                        vLinhaArg[iy++] = toupper(*blin);
                }
                else
                {
                    for(kt = 0; kt < ix; kt++)
                        vLinhaArg[iy++] = vLido[kt];

                    if (*blin && *blin != 0x20)
                        vLinhaArg[iy++] = toupper(*blin);
                }
            }
            else
            {
                if (*blin && *blin != 0x20)
                    vLinhaArg[iy++] = toupper(*blin);
            }

            if (!*blin)
                break;

            vLido[0] = '\0';
            ix = 0;
        }
        else
        {
            if (!vAspas && !vTemRem)
                vLido[ix++] = toupper(*blin);
            else
                vLido[ix++] = *blin;
        }

        blin++;
    }

    vLinhaArg[iy] = 0x00;

    for(kt = 0; kt < iy; kt++)
    {
        pTokenized[kt] = vLinhaArg[kt];
    }

    pTokenized[iy] = 0x00;
}

//-----------------------------------------------------------------------------
// Salva a linha no formato:
// NN NN NN NN LL LL xxxxxxxxxxxx 00
// onde:
//      NN NN NN NN         = endereco da proxima linha
//      LL LL            = Numero da linha
//      xxxxxxxxxxxxxx   = Linha Tokenizada
//      00               = Indica fim da linha
//-----------------------------------------------------------------------------
int saveLine(char *pNumber, unsigned char *pTokenized)
{
    unsigned short vTam = 0, kt;
    unsigned char *pSave = (unsigned char*)nextAddrLine;
    unsigned long vNextAddr = 0, vAntAddr = 0, vNextAddr2 = 0;
    unsigned short vNumLin = 0;
    unsigned char *pAtu = (unsigned char*)nextAddrLine, *pLast = (unsigned char*)nextAddrLine;

    vNumLin = atoi(pNumber);

    if (firstLineNumber == 0)
    {
        firstLineNumber = vNumLin;
        addrFirstLineNumber = (unsigned long)pStartProg;
    }
    else
    {
        vNextAddr = findNumberLine(vNumLin, 0, 0);

        if (vNextAddr > 0)
        {
            pAtu = (unsigned char*)vNextAddr;

            if (((*(pAtu + 4) << 8) | *(pAtu + 5)) == vNumLin)
            {
                println((char*)"Line number already exists\0");
                return 0xFF02;
            }

            vAntAddr = findNumberLine(vNumLin, 1, 0);
        }
    }

    vTam = strlen((char*)pTokenized);
    if (vTam)
    {
        // Calcula nova posicao da proxima linha
        if (vNextAddr == 0)
        {
            nextAddrLine += (vTam + 7);
            vNextAddr = nextAddrLine;

            addrLastLineNumber = (unsigned long)pSave;
        }
        else
        {
            if (firstLineNumber > vNumLin)
            {
                firstLineNumber = vNumLin;
                addrFirstLineNumber = nextAddrLine;
            }

            nextAddrLine += (vTam + 7);
            vNextAddr2 = nextAddrLine;

            if (vAntAddr != vNextAddr)
            {
                pLast = (unsigned char*)vAntAddr;
                vAntAddr = (unsigned long)pSave;
                *pLast       = ((vAntAddr & 0xFF000000) >> 24);
                *(pLast + 1) = ((vAntAddr & 0xFF0000) >> 16);
                *(pLast + 2) = ((vAntAddr & 0xFF00) >> 8);
                *(pLast + 3) =  (vAntAddr & 0xFF);
            }

            pLast = (unsigned char*)addrLastLineNumber;
            *pLast       = ((vNextAddr2 & 0xFF000000) >> 24);
            *(pLast + 1) = ((vNextAddr2 & 0xFF0000) >> 16);
            *(pLast + 2) = ((vNextAddr2 & 0xFF00) >> 8);
            *(pLast + 3) =  (vNextAddr2 & 0xFF);
        }

        pAtu = (unsigned char*)nextAddrLine;
        *pAtu       = 0x00;
        *(pAtu + 1) = 0x00;
        *(pAtu + 2) = 0x00;
        *(pAtu + 3) = 0x00;
        *(pAtu + 4) = 0x00;
        *(pAtu + 6) = 0x00;

        // Grava endereco proxima linha
        *pSave++ = ((vNextAddr & 0xFF000000) >> 24);
        *pSave++ = ((vNextAddr & 0xFF0000) >> 16);
        *pSave++ = ((vNextAddr & 0xFF00) >> 8);
        *pSave++ =  (vNextAddr & 0xFF);

        // Grava numero da linha
        *pSave++ = ((vNumLin & 0xFF00) >> 8);
        *pSave++ = (vNumLin & 0xFF);

        // Grava linha *tokenizada
        for(kt = 0; kt < vTam; kt++)
            *pSave++ = *pTokenized++;

        // Grava final linha 0x00        
        *pSave = 0x00;

        pSave = (unsigned char*)vNextAddr;
        *pSave = 0x00;
        *(pSave + 1) = 0x00;
        *(pSave + 2) = 0x00;
        *(pSave + 3) = 0x00;
        *(pSave + 4) = 0x00;
        *(pSave + 5) = 0x00;
    }

    return 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
unsigned long findNumberLine(unsigned short pNumber, unsigned char pTipoRet, unsigned char pTipoFind)
{
    unsigned char *vStartList = (unsigned char*)addrFirstLineNumber;
    unsigned char *vLastList = (unsigned char*)addrFirstLineNumber;
    unsigned short vNumber = 0;

    if (pNumber)
    {
        while(vStartList)
        {
            vNumber = ((*(vStartList + 4) << 8) | *(vStartList + 5));

            if ((!pTipoFind && vNumber < pNumber) || (pTipoFind && vNumber != pNumber))
            {
                vLastList = vStartList;
                vStartList = (unsigned char*)((unsigned long)(*(vStartList) << 24) | (unsigned long)(*(vStartList + 1) << 16) | (unsigned long)(*(vStartList + 2) << 8) | (unsigned long)*(vStartList + 3));
            }
            else
                break;
        }
    }

    if (!pTipoRet)
        return (unsigned long)vStartList;
    else
        return (unsigned long)vLastList;
}

//-----------------------------------------------------------------------------
// Sintaxe:
//      RUN                : Executa o programa a partir da primeira linha do prog
//      RUN <num>          : Executa a partir da linha <num>
//-----------------------------------------------------------------------------
void runProg(unsigned char *pNumber)
{
    // Default rodar desde a primeira linha
    int pIni = 0, ix;
    unsigned char *vStartList = pStartProg;
    unsigned long vNextList;
    unsigned short vNumLin;
//    typeInf vRetInf;
    char sNumLin [sizeof(short)*8+1];
    unsigned char *vPointerChangedPointer;
    unsigned char *vTempPointer;

    nextAddrSimpVar = (unsigned long)pStartSimpVar;
    nextAddrArrayVar = (unsigned long)pStartArrayVar;
    nextAddrString = (unsigned long)pStartString;

    for (ix = 0; ix < BASIC_SIZE_VAR; ix++)
        *(pStartSimpVar + ix) = 0x00;

    for (ix = 0; ix < BASIC_SIZE_ARRAY_VAR; ix++)
        *(pStartArrayVar + ix) = 0x00;

    for (ix = 0; ix < FOR_NEST; ix++)
    {
        forStack[ix].nameVar[0] = 0;
        forStack[ix].nameVar[1] = 0;
        forStack[ix].nameVar[2] = 0;
    }

    if (pNumber[0] != 0x00)
    {
        // rodar desde uma linha especifica
        pIni = atoi((char*)pNumber);
    }

    clearScr();
    println((char*)"Running...");

    vStartList = (unsigned char*)findNumberLine(pIni, 0, 0);

    // Nao achou numero de linha inicial
    if (!vStartList)
    {
        println((char*)"Non-existent line number");
        return;
    }

    vNextList = (unsigned long)vStartList;

    ftos=0;
    gtos=0;
    changedPointer = 0;
    *vDataPointer = 0;
    randSeed = 1; // Pegar Temp do computador
    onErrGoto = 0;
    traceOn=0;
    vBufReceived=0;

    while (1)
    {
        if (changedPointer!=0)
            vStartList = (unsigned char*)changedPointer;

        // Guarda proxima posicao
        vNextList = (*(vStartList) << 24) | (*(vStartList + 1) << 16) | (*(vStartList + 2) << 8) | *(vStartList + 3);
        nextAddr = vNextList;

        if (vStartList)
        {
            // Pega numero da linha
            vNumLin = (*(vStartList + 4) << 8) | *(vStartList + 5);

            if (vNumLin == 0)
                break;

            vStartList += 6;

            // Pega caracter a caracter da linha
            changedPointer = 0;
            vParenteses = 0x00;
//            vRetInf.tString[0] = 0x00;

            pointerRunProg = (unsigned long)vStartList;
//            token = (unsigned char*)pointerRunProg;

            vErroProc = 0;

            do
            {
                vBufReceived = readChar(KEY_NOWAIT);
                if (vBufReceived==27)
                {
                    // mostra mensagem de para subita
                    println((char*)"");
                    print((char*)"Stopped at ");
                    itoa(vNumLin, sNumLin, 10);
                    print(sNumLin);
                    println((char*)"");                    

                    // sai do laço
                    nextAddr = 0;
                    break;
                }

                doisPontos = 0;
                vParenteses = 0x00;
                vInicioSentenca = 1;

                if (traceOn)
                {
//                    clearScr();
                    println((char*)"");
                    print((char*)"Executing at ");
                    itoa(vNumLin, sNumLin, 10);
                    print(sNumLin);
                    println((char*)"");
                }

                vTempPointer = (unsigned char*)pointerRunProg;
                pointerRunProg = pointerRunProg + 1;
                executeToken(*vTempPointer);

                if (vErroProc)
                {
                    if (onErrGoto == 0)
                        break;

                    vErroProc = 0;
                    changedPointer = onErrGoto;
                }

                if (changedPointer!=0)
                {
                    vPointerChangedPointer = (unsigned char*)changedPointer;

                    if (*vPointerChangedPointer == 0x3A)
                    {
                        pointerRunProg = changedPointer;
                        changedPointer = 0;
                    }
                }

                vTempPointer = (unsigned char*)pointerRunProg;
                if (*vTempPointer != 0x00)
                {
                    if (*vTempPointer == 0x3A)
                    {
                        doisPontos = 1;
                        pointerRunProg = pointerRunProg + 1;
                    }
                    else
                    {
                        if (doisPontos && *vTempPointer <= 0x80)
                        {
                            // nao faz nada
                        }
                        else
                        {
                            nextToken();
                            if (vErroProc) break;
                        }
                    }
                }
            } while (doisPontos);

            if (vErroProc)
            {
                showErrorMessage(vErroProc, vNumLin);
                break;
            }

            if (nextAddr == 0)
                break;

            vNextList = nextAddr;

            vStartList = (unsigned char*)vNextList;
        }
        else
            break;
    }

    #ifdef IN_TOUCH
    waitTouch();
    #endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void showErrorMessage(unsigned int pError, unsigned int pNumLine)
{
    char sNumLin [sizeof(short)*8+1];

    println((char*)"");
    print(listError[pError]);

    if (pNumLine > 0)
    {
        itoa(pNumLine, sNumLin, 10);

        print((char*)" at ");
        print(sNumLin);
    }

    println((char*)" !");

    vErroProc = 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int executeToken(unsigned char pToken)
{
    char vReta = 0;
#ifndef __TESTE_TOKENIZE__
    int ix;
    for_stack forclear;

    switch (pToken)
    {
        case 0x00:  // End of Line
            vReta = 0;
            break;
        case 0x80:  // Let
            vReta = basLet();
            break;
        case 0x81:  // Print
            vReta = basPrint();
            break;
        case 0x82:  // IF
            vReta = basIf();
            break;
        case 0x83:  // THEN - nao faz nada
            vReta = 0;
            break;
        case 0x85:  // FOR
            vReta = basFor();
            break;
        case 0x86:  // TO - nao faz nada
            vReta = 0;
            break;
        case 0x87:  // NEXT
            vReta = basNext();
            break;
        case 0x88:  // STEP - nao faz nada
            vReta = 0;
            break;
        case 0x89:  // GOTO
            vReta = basGoto();
            break;
        case 0x8A:  // GOSUB
            vReta = basGosub();
            break;
        case 0x8B:  // RETURN
            vReta = basReturn();
            break;
        case 0x8C:  // REM - Ignora todas a linha depois dele
            vReta = 0;
            break;
        case 0x8D:  // INVERSE
            vReta = basInverse();
            break;
        case 0x8E:  // NORMAL
            vReta = basNormal();
            break;
        case 0x8F:  // DIM
            vReta = basDim();
            break;
        case 0x90:  // Nao fax nada, soh teste, pode ser retirado
            vReta = 0;
            break;
        case 0x91:  // DIM
            vReta = basOnVar();
            break;
        case 0x92:  // Input
            vReta = basInputGet(250);
            break;
        case 0x93:  // Get
            vReta = basInputGet(1);
            break;
        case 0x94:  // vTAB
            vReta = basVtab();
            break;
        case 0x95:  // HTAB
            vReta = basHtab();
            break;
        case 0x96:  // Home
            clearScr();
            break;
        case 0x97:  // CLEAR - Clear all variables
            for (ix = 0; ix < BASIC_SIZE_VAR; ix++)
                *(pStartSimpVar + ix) = 0x00;

            for (ix = 0; ix < BASIC_SIZE_ARRAY_VAR; ix++)
                *(pStartArrayVar + ix) = 0x00;

            forclear.nameVar[0] = '\0';

            for (ix = 0; ix < FOR_NEST; ix++)
                forStack[ix] = forclear;

            vReta = 0;
            break;
        case 0x98:  // DATA - Ignora toda a linha depois dele, READ vai ler essa linha
            vReta = 0;
            break;
        case 0x99:  // Read
            vReta = basRead();
            break;
        case 0x9A:  // Restore
            vReta = basRestore();
            break;
        case 0x9E:  // END
            vReta = basEnd();
            break;
        case 0x9F:  // STOP
            vReta = basStop();
            break;
        case 0xB0:  // TEXT
            vReta = basText();
            break;
        case 0xB1:  // GR
            vReta = basGr();
            break;
        case 0xB2:  // HGR
            vReta = basHgr();
            break;
        case 0xB3:  // COLOR
            vReta = basColor();
            break;
        case 0xB4:  // PLOT
            vReta = basPlot();
            break;
        case 0xB5:  // HLIN
            vReta = basHVlin(1);
            break;
        case 0xB6:  // VLIN
            vReta = basHVlin(2);
            break;
        case 0xB8:  // HCOLOR
            vReta = basHcolor();
            break;
        case 0xB9:  // HPLOT
            vReta = basHplot();
            break;
        case 0xBA:  // AT - Nao faz nada
            vReta = 0;
            break;
        case 0xBB:  // ONERR
            vReta = basOnErr();
            break;
        case 0xC4:  // ASC
            vReta = basAsc();
            break;
        case 0xCD:  // PEEK
            vReta = basPeekPoke('R');
            break;
        case 0xCE:  // POKE
            vReta = basPeekPoke('W');
            break;
        case 0xD1:  // RND
            vReta = basRnd();
            break;
        case 0xDB:  // Len
            vReta = basLen();
            break;
        case 0xDC:  // Val
            vReta = basVal();
            break;
        case 0xDD:  // Str$
            vReta = basStr();
            break;
        case 0xE0:  // SCRN
            vReta = basScrn();
            break;
        case 0xE1:  // Chr$
            vReta = basChr();
            break;
        case 0xE2:  // Fre(0)
            vReta = basFre();
            break;
        case 0xE3:  // Sqrt
            vReta = basTrig(6);
            break;
        case 0xE4:  // Sin
            vReta = basTrig(1);
            break;
        case 0xE5:  // Cos
            vReta = basTrig(2);
            break;
        case 0xE6:  // Tan
            vReta = basTrig(3);
            break;
        case 0xE7:  // Log
            vReta = basTrig(4);
            break;
        case 0xE8:  // Exp
            vReta = basTrig(5);
            break;
        case 0xE9:  // SPC
            vReta = basSpc();
            break;
        case 0xEA:  // Tab
            vReta = basTab();
            break;
        case 0xEB:  // Mid$
            vReta = basLeftRightMid('M');
            break;
        case 0xEC:  // Right$
            vReta = basLeftRightMid('R');
            break;
        case 0xED:  // Left$
            vReta = basLeftRightMid('L');
            break;
        case 0xEE:  // INT
            vReta = basInt();
            break;
        case 0xEF:  // ABS
            vReta = basAbs();
            break;
        default:
            if (pToken < 0x80)  // variavel sem LET
            {
                pointerRunProg = pointerRunProg - 1;
                vReta = basLet();
            }
            else // Nao forem operadores logicos
            {
                vErroProc = 14;
                vReta = 14;
            }
    }
#endif
    return vReta;
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
int nextToken(void)
{
    unsigned char *temp = token;
    unsigned char *vTempPointer;

    token_type = 0;
    tok = 0;

    vTempPointer = (unsigned char*)pointerRunProg;
    if (*vTempPointer >= 0x80 && *vTempPointer < 0xF0)   // is a command
    {
        tok = *vTempPointer;
        token_type = COMMAND;
        *token = *vTempPointer;
        *(token + 1) = 0x00;

        return token_type;
    }

    if (*vTempPointer == '\0') { // end of file
        *token = 0;
        tok = FINISHED;
//if (debugON) print((char*)".Aqui 3.");        
        token_type = DELIMITER;

        return token_type;
    }

    while(iswhite(*vTempPointer)) // skip over white space
    {
        pointerRunProg = pointerRunProg + 1;
        vTempPointer = (unsigned char*)pointerRunProg;
    }

    if (*vTempPointer == '\r') { // crlf
        pointerRunProg = pointerRunProg + 2;
        tok = EOL;
        *token = '\r';
        *(token + 1) = '\n';
        *(token + 2) = 0;
        token_type = DELIMITER;

        return token_type;
    }

    if (strchr("+-*^/=;:,><", *vTempPointer) || *vTempPointer >= 0xF0) { // delimiter
        *temp = *vTempPointer;
        pointerRunProg = pointerRunProg + 1; // advance to next position
        temp++;
        *temp = 0;

        token_type = DELIMITER;

        return token_type;
    }

    if (*vTempPointer == 0x28 || *vTempPointer == 0x29)
    {
        if (*vTempPointer == 0x28)
            token_type = OPENPARENT;
        else
            token_type = CLOSEPARENT;

        *token = *vTempPointer;
        *(token + 1) = 0x00;

        pointerRunProg = pointerRunProg + 1;

        return token_type;
    }

    if (*vTempPointer == ':')
    {
        doisPontos = 1;
        token_type = DOISPONTOS;

        return token_type;
    }

    if (*vTempPointer == '"') { // quoted string
        pointerRunProg = pointerRunProg + 1;
        vTempPointer = (unsigned char*)pointerRunProg;

        while(*vTempPointer != '"'&& *vTempPointer != '\r')
        {
            *temp++ = *vTempPointer;
            pointerRunProg = pointerRunProg + 1;
            vTempPointer = (unsigned char*)pointerRunProg;
        }

        if (*vTempPointer == '\r')
        {
            vErroProc = 14;
            return 0;
        }

        pointerRunProg = pointerRunProg + 1;
        *temp = 0;
        token_type = QUOTE;

        return token_type;
    }
    if (isdigitus(*vTempPointer)) { // number

        while(!isdelim(*vTempPointer) && (*vTempPointer < 0x80 || *vTempPointer >= 0xF0))
        {
            *temp++ = *vTempPointer;
            pointerRunProg = pointerRunProg + 1;
            vTempPointer = (unsigned char*)pointerRunProg;
        }
        *temp = '\0';
        token_type = NUMBER;

        return token_type;
    }

    if (isalphas(*vTempPointer)) { // var or command
        while(!isdelim(*vTempPointer) && (*vTempPointer < 0x80 || *vTempPointer >= 0xF0))
        {
            *temp++ = *vTempPointer;
            pointerRunProg = pointerRunProg + 1;
            vTempPointer = (unsigned char*)pointerRunProg;
        }

        *temp = '\0';
        token_type = VARIABLE;

        return token_type;
    }

    *temp = '\0';

    // see if a string is a command or a variable
    if (token_type == STRING) {
        token_type = VARIABLE;
    }

    return token_type;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int findToken(unsigned char pToken)
{
    unsigned char kt;

    // Procura o Token na lista e devolve a posicao
    for(kt = 0; kt < keywords_count; kt++)
    {
        if (keywords[kt].token == pToken)
            return kt;
    }

    // Procura o Token nas operacões de 1 char
    /*for(kt = 0; kt < keywordsUnique_count; kt++)
    {
        if (keywordsUnique[kt].*token == pToken)
            return (kt + 0x80);
    }*/

    return 14;
}

//--------------------------------------------------------------------------------------
// Return true if c is a alphabetical (A-Z or a-z).
//--------------------------------------------------------------------------------------
int isalphas(unsigned char c)
{
    if ((c>0x40 && c<0x5B) || (c>0x60 && c<0x7B))
       return 1;

    return 0;
}

//--------------------------------------------------------------------------------------
// Return true if c is a number (0-9).
//--------------------------------------------------------------------------------------
int isdigitus(unsigned char c)
{
    if (c>0x2F && c<0x3A)
       return 1;

    return 0;
}

//--------------------------------------------------------------------------------------
// Return true if c is a delimiter.
//--------------------------------------------------------------------------------------
int isdelim(unsigned char c)
{
    if (strchr(" ;,+-<>()/*^=:", c) || c==9 || c=='\r' || c==0 || c>=0xF0)
       return 1;

    return 0;
}

//--------------------------------------------------------------------------------------
// Return 1 if c is space or tab.
//--------------------------------------------------------------------------------------
int iswhite(unsigned char c)
{
    if (c==' ' || c=='\t')
       return 1;

    return 0;
}

//-----------------------------------------------------------------------------
// Retornos: -1 - Erro, 0 - Nao Existe, 1 - eh um valor numeral
//           [endereco > 1] - Endereco da variavel
//
//           se retorno > 1: pVariable vai conter o valor numeral (qdo 1) ou
//                           o conteudo da variavel (qdo endereco)
//-----------------------------------------------------------------------------
long findVariable(unsigned char* pVariable)
{
    unsigned char* vLista = pStartSimpVar;
    unsigned char* vTemp = pStartSimpVar;
    long vEnder = 0;
    int ix = 0, iy = 0, iz = 0, iw = 0;
    unsigned char vDim[88];
    unsigned char vTempDim[10];
    unsigned long vOffSet;
    unsigned char ixDim = 0;
    unsigned char vArray = 0;
    unsigned long vPosNextVar = 0;
    unsigned char* vPosValueVar = 0;
    unsigned char vTamValue = 4;
    unsigned char *vTempPointer;
    unsigned short iDim = 0;
    union uValue uFindV;
    float fValue;
    int iValue;

    // Verifica se eh array (tem parenteses logo depois do nome da variavel)

    vTempPointer = (unsigned char*)pointerRunProg;
    if (*vTempPointer == 0x28)
    {
        // Define que eh array
        vArray = 1;

        // Procura as dimensoes
        nextToken();
        if (vErroProc) return 0;

        // Erro, primeiro caracter depois da variavel, deve ser abre parenteses
        if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
        {
            vErroProc = 15;
            return 0;
        }

        do
        {
            nextToken();
            if (vErroProc) return 0;

            if (token_type == QUOTE) { // is string, error
                vErroProc = 16;
                return 0;
            }
            else { // is expression
                putback();

                getExp(vTempDim);

                if (vErroProc) return 0;

                if (value_type == '$')
                {
                    vErroProc = 16;
                    return 0;
                }

                memcpy(&uFindV, vTempDim, 4);

                if (value_type == '#')
                {
                    fValue = uFindV.fVal;
                    iValue = (int)fValue;
                    value_type = '%';
                }
                else
                {
                    iValue = uFindV.iVal;
                }

                vDim[ixDim] = (char)iValue + 1;
                ixDim++;

            }

            if (*token == ',')
            {
                pointerRunProg = pointerRunProg + 1;
                vTempPointer = (unsigned char*)pointerRunProg;
            }
            else
                break;

        } while(1);

        // Deve ter pelo menos 1 elemento
        if (ixDim < 1)
        {
            vErroProc = 21;
            return 0;
        }

        nextToken();
        if (vErroProc) return 0;

        // Ultimo caracter deve ser fecha parenteses
        if (token_type!=CLOSEPARENT)
        {
            vErroProc = 15;
            return 0;
        }
    }

    // Procura na lista geral de variaveis simples / array
    if (vArray)
        vLista = pStartArrayVar;
    else
        vLista = pStartSimpVar;

    while(1)
    {
        vPosNextVar  = (((unsigned long)*(vLista + 3) << 24) & 0xFF000000);
        vPosNextVar |= (((unsigned long)*(vLista + 4) << 16) & 0x00FF0000);
        vPosNextVar |= (((unsigned long)*(vLista + 5) << 8) & 0x0000FF00);
        vPosNextVar |= ((unsigned long)*(vLista + 6) & 0x000000FF);
        value_type = *vLista;

        if (*(vLista + 1) == pVariable[0] && *(vLista + 2) ==  pVariable[1])
        {
            // Pega endereco da variavel pra delvover
            if (vArray)
            {
                if (*vLista == '$')
                    vTamValue = 5;

                // Verifica se os tamanhos da dimensao informada e da variavel sao iguais
                if (ixDim != vLista[7])
                {
                    vErroProc = 21;
                    return 0;
                }

                // Verifica se as posicoes informadas sao iguais ou menores que as da variavel, e ja calcula a nova posicao
                iw = (ixDim - 1);
                iz = 0;
                for (ix = ((ixDim - 1) * 2 ); ix >= 0; ix -= 2)
                {
                    // Verifica tamanho posicao
                    iDim = ((vLista[ix + 8] << 8) | vLista[ix + 9]);

                    if (vDim[iw] > iDim)
                    {
                        vErroProc = 21;
                        return 0;
                    }

                    // Calcular a posicao do conteudo da variavel
                    vPosValueVar = vPosValueVar + (powNum(iDim, iz) * (vDim[iw] - 1 ) * vTamValue);

                    iw--;
                }

                vOffSet = (unsigned long)vLista;
                vPosValueVar = vPosValueVar + (vOffSet + 8 + (ixDim * 2));
                vEnder = (long)vPosValueVar;
            }
            else
            {
                vPosValueVar = vLista + 3;
                vEnder = (long)vLista;
            }

            // Pelo tipo da variavel, ja retorna na variavel de nome o conteudo da variavel
            if (*vLista == '$')
            {
                vOffSet  = (((unsigned long)*(vPosValueVar + 1) << 24) & 0xFF000000);
                vOffSet |= (((unsigned long)*(vPosValueVar + 2) << 16) & 0x00FF0000);
                vOffSet |= (((unsigned long)*(vPosValueVar + 3) << 8) & 0x0000FF00);
                vOffSet |= ((unsigned long)*(vPosValueVar + 4) & 0x000000FF);
                vTemp = (unsigned char*)vOffSet;

                iy = *vPosValueVar;
                iz = 0;

                for (ix = 0; ix < iy; ix++)
                {
                    pVariable[iz++] = *(vTemp + ix); // Numero gerado
                    pVariable[iz] = 0x00;
                }

                pVariable[iz++] = 0x00;
            }
            else
            {
                if (!vArray)
                    vPosValueVar++;

                pVariable[0] = *(vPosValueVar);
                pVariable[1] = *(vPosValueVar + 1);
                pVariable[2] = *(vPosValueVar + 2);
                pVariable[3] = *(vPosValueVar + 3);
                pVariable[4] = 0x00;
            }

            return vEnder;
        }

        if (vArray)
            vLista = (unsigned char*)vPosNextVar;
        else
            vLista += 8;

        if ((!vArray && vLista >= pStartArrayVar) || (vArray && vLista >= pStartProg) || *vLista == 0x00)
            break;
    }

    return 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
char createVariable(unsigned char* pVariable, unsigned char* pValor, char pType)
{
    char vRet = 0;
    unsigned char* vNextSimpVar;

//    vTemp = nextAddrSimpVar;
    vNextSimpVar = (unsigned char*)nextAddrSimpVar;

//    vLenVar = strlen((char*)pVariable);

    *vNextSimpVar++ = pType;
    *vNextSimpVar++ = pVariable[0];
    *vNextSimpVar++ = pVariable[1];

    vRet = updateVariable((unsigned long*)vNextSimpVar, pValor, pType, 0);
    nextAddrSimpVar += 8;

    return vRet;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
char updateVariable(unsigned long* pVariable, unsigned char* pValor, char pType, char pOper)
{
    long vNumVal = 0;
    int ix, iz = 0;
    unsigned char* vNextSimpVar;
    unsigned char* vNextString;
    char pNewStr = 0;
    unsigned long vOffSet;
//    unsigned char* sqtdtam[20];

    vNextSimpVar = (unsigned char*)pVariable;
    atuVarAddr = (unsigned long)pVariable;

    if (pType == '#' || pType == '%')   // Real ou Inteiro
    {
        if (vNextSimpVar < pStartArrayVar)
            *vNextSimpVar++ = 0x00;

        *vNextSimpVar++ = pValor[0];
        *vNextSimpVar++ = pValor[1];
        *vNextSimpVar++ = pValor[2];
        *vNextSimpVar++ = pValor[3];
    }
    else // String
    {
        iz = strlen((char*)pValor);    // Tamanho da strings

        // Se for o mesmo tamanho ou menor, usa a mesma posicao
        if (*vNextSimpVar <= iz && pOper)
        {
            vOffSet  = (((unsigned long)*(vNextSimpVar + 1) << 24) & 0xFF000000);
            vOffSet |= (((unsigned long)*(vNextSimpVar + 2) << 16) & 0x00FF0000);
            vOffSet |= (((unsigned long)*(vNextSimpVar + 3) << 8) & 0x0000FF00);
            vOffSet |= ((unsigned long)*(vNextSimpVar + 4) & 0x000000FF);
            vNextString = (unsigned char*)vOffSet;

            if (pOper == 2 && vNextString == 0)
            {
                vNextString = (unsigned char*)nextAddrString;
                pNewStr = 1;
            }
        }
        else
            vNextString = (unsigned char*)nextAddrString;

        vNumVal = (long)vNextString;

        for (ix = 0; ix < iz; ix++)
        {
            *vNextString++ = pValor[ix];
        }

        if (*vNextSimpVar > iz || !pOper || pNewStr)
            nextAddrString = (unsigned long)vNextString;

        *vNextSimpVar++ = iz;

        *vNextSimpVar++ = ((vNumVal & 0xFF000000) >>24);
        *vNextSimpVar++ = ((vNumVal & 0x00FF0000) >>16);
        *vNextSimpVar++ = ((vNumVal & 0x0000FF00) >>8);
        *vNextSimpVar++ = (vNumVal & 0x000000FF);
    }

    return 0;
}

//--------------------------------------------------------------------------------------
char createVariableArray(unsigned char* pVariable, char pType, unsigned int pNumDim, unsigned int *pDim)
{
    long vTemp = 0;
    unsigned char* vNextArrayVar;
    unsigned int ix, vTam;
    long vAreaFree = (pStartString - (unsigned char*)nextAddrArrayVar);
    long vSizeTotal = 0;
//    unsigned char sqtdtam[20];

    vTemp = nextAddrArrayVar;
    vNextArrayVar = (unsigned char*)nextAddrArrayVar;

//    vLenVar = strlen((char*)pVariable);

    *vNextArrayVar++ = pType;
    *vNextArrayVar++ = pVariable[0];
    *vNextArrayVar++ = pVariable[1];
    vTam = 0;

    for (ix = 0; ix < pNumDim; ix++)
    {
        // Somando mais 1, porque 0 = 1 em quantidade e e em posicao (igual ao c)
        pDim[ix] = pDim[ix] /*+ 1*/ ;

        // Definir o tamanho do campo de dados do array
        if (vTam == 0)
            vTam = pDim[ix] /*- 1*/ ;
        else
            vTam = vTam * (pDim[ix] /*- 1*/ );
    }

    if (pType == '$')
        vTam = vTam * 5;
    else
        vTam = vTam * 4;

    vSizeTotal = vTam + 8;
    vSizeTotal = vSizeTotal + (pNumDim *2);

    if (vSizeTotal > vAreaFree)
    {
        vErroProc = 22;
        return 0;
    }

    // Coloca setup do array
    vTemp = vTemp + vTam + 8 + (pNumDim * 2);
    *vNextArrayVar++ = (unsigned char)((vTemp >> 24) & 0x000000FF);
    *vNextArrayVar++ = (unsigned char)((vTemp >> 16) & 0x000000FF);
    *vNextArrayVar++ = (unsigned char)((vTemp >> 8) & 0x000000FF);
    *vNextArrayVar++ = (unsigned char)((vTemp) & 0x000000FF);
    *vNextArrayVar++ = pNumDim;

    for (ix = 0; ix < pNumDim; ix++)
    {
        *vNextArrayVar++ = (pDim[ix] >> 8);
        *vNextArrayVar++ = (pDim[ix] & 0xFF);
    }

    // Limpa area de dados (zera)
    for (ix = 0; ix < vTam; ix++)
        *(vNextArrayVar + ix) = 0x00;

    nextAddrArrayVar = vTemp;

    return 0;
}

//--------------------------------------------------------------------------------------
// Return a *token to input stream.
//--------------------------------------------------------------------------------------
void putback(void)
{
    unsigned char *t;

    if (token_type==COMMAND)    // comando nao faz isso
        return;

    t = token;
    while (*t++)
        pointerRunProg = pointerRunProg - 1;
}

//--------------------------------------------------------------------------------------
// Return compara 2 strings
//--------------------------------------------------------------------------------------
int ustrcmp(char *X, char *Y)
{
    while (*X)
    {
        // if characters differ, or end of the second string is reached
        if (*X != *Y) {
            break;
        }

        // move to the next pair of characters
        X++;
        Y++;
    }

    // return the ASCII difference after converting `char*` to `unsigned char*`
    return *(unsigned char*)X - *(unsigned char*)Y;
}

//--------------------------------------------------------------------------------------
// Entry point into parser.
//--------------------------------------------------------------------------------------
void getExp(unsigned char *result)
{
//if (debugON2) sprintf(dataShow, ".Aqui.1.%lX.", pointerRunProg);
//if (debugON2) print(dataShow);
    nextToken();
    if (vErroProc) return;
//if (debugON2) sprintf(dataShow, ".Aqui.2.%lX.", pointerRunProg);
//if (debugON2) print(dataShow);

    if (!*token) {
        vErroProc = 2;
        return;
    }

    level2(result);
    if (vErroProc) return;

    putback(); // return last *token read to input stream

    return;
}

//--------------------------------------------------------------------------------------
//  Add or subtract two terms real/int or string.
//--------------------------------------------------------------------------------------
void level2(unsigned char *result)
{
    char  op;
    unsigned char hold[50];
    unsigned char valueTypeAnt;
    union uValue resValue;
    union uValue holValue;
    float fValue;
//if (debugON3) print((char*)".Aquilevel2.");

    level3(result);
    if (vErroProc) return;

    op = *token;

    while(op == '+' || op == '-') {
        nextToken();
        if (vErroProc) return;

        valueTypeAnt = value_type;

        level3(hold);
        if (vErroProc) return;

        if (value_type != valueTypeAnt)
        {
            if (value_type == '$' || valueTypeAnt == '$')
            {
                vErroProc = 16;
                return;
            }
        }

        // Se forem diferentes os 2, se for um deles string, da erro, se nao, passa o inteiro para real
        if (value_type == '$' && valueTypeAnt == '$' && op == '+')
            strcat((char*)result,(char*)hold);
        else if ((value_type == '$' || valueTypeAnt == '$') && op == '-')
        {
            vErroProc = 16;
            return;
        }
        else
        {
            if (value_type != valueTypeAnt)
            {
                memcpy(&resValue, result, 4);
                memcpy(&holValue, hold, 4);

                if (value_type == '$' || valueTypeAnt == '$')
                {
                    vErroProc = 16;
                    return;
                }
                else if (value_type == '#')
                {
                    fValue = (float)resValue.iVal;
                    resValue.fVal = fValue;
                }
                else
                {
                    fValue = (float)holValue.iVal;
                    holValue.fVal = fValue;
                    value_type = '#';
                }

                memcpy(result, resValue.vBytes, 4);
                memcpy(hold, holValue.vBytes, 4);
            }

            if (value_type == '#')
                arithReal(op, (char*)result, (char*)hold);
            else
                arithInt(op, (char*)result, (char*)hold);
        }

        op = *token;
    }

    return;
}

//--------------------------------------------------------------------------------------
// Multiply or divide two factors real/int.
//--------------------------------------------------------------------------------------
void level3(unsigned char *result)
{
    char  op;
    unsigned char hold[50];
    char value_type_ant=0;
    union uValue resValue;
    union uValue holValue;
    float fValue;
//if (debugON3) print((char*)".Aquilevel3.");

    do
    {
        level30(result);
        if (vErroProc) return;
        if (*token==0xF3||*token==0xF4)
        {
            nextToken();
            if (vErroProc) return;
        }
        else
            break;
    }
    while (1);

    op = *token;
    while(op == '*' || op == '/' || op == '%') {
        if (value_type == '$')
        {
            vErroProc = 16;
            return;
        }

        nextToken();
        if (vErroProc) return;

        value_type_ant = value_type;

        level4(hold);
        if (vErroProc) return;

        // Se forem diferentes os 2, se for um deles string, da erro, se nao, passa o inteiro para real
        if (value_type == '$' || value_type_ant == '$')
        {
            vErroProc = 16;
            return;
        }

        if (value_type != value_type_ant)
        {
            memcpy(&resValue, result, 4);
            memcpy(&holValue, hold, 4);

            if (value_type == '#')
            {
                fValue = (float)resValue.iVal;
                resValue.fVal = fValue;
            }
            else
            {
                fValue = (float)holValue.iVal;
                holValue.fVal = fValue;
                value_type = '#';
            }

            memcpy(result, resValue.vBytes, 4);
            memcpy(hold, holValue.vBytes, 4);
        }

        // se valor inteiro e for divisao, obrigatoriamente devolve valor real
        if (value_type == '%' && op == '/')
        {
            memcpy(&resValue, result, 4);
            memcpy(&holValue, hold, 4);

            fValue = (float)resValue.iVal;
            resValue.fVal = fValue;
            fValue = (float)holValue.iVal;
            holValue.fVal = fValue;
            value_type = '#';

            memcpy(result, resValue.vBytes, 4);
            memcpy(hold, holValue.vBytes, 4);
        }

        if (value_type == '#')
            arithReal(op, (char*)result, (char*)hold);
        else
            arithInt(op, (char*)result, (char*)hold);

        op = *token;
    }

    return;
}

//--------------------------------------------------------------------------------------
// Is a NOT
//--------------------------------------------------------------------------------------
void level30(unsigned char *result)
{
    char  op;
//if (debugON3) print((char*)".Aquilevel30.");

    op = 0;
    if (*token == 0xF8) // NOT
    {
        op = *token;
        nextToken();
        if (vErroProc) return;
    }

    level31(result);
    if (vErroProc) return;

    if (op)
    {
        if (value_type == '$' || value_type == '#')
        {
            vErroProc = 16;
            return;
        }

        *result = !*result;
    }

    return;
}

//--------------------------------------------------------------------------------------
// Process logic conditions
//--------------------------------------------------------------------------------------
void level31(unsigned char *result)
{
    unsigned char  op;
    unsigned char hold[50];
//if (debugON3) print((char*)".Aquilevel31.");

    level32(result);
    if (vErroProc) return;

    op = *token;
    if (op==0xF3 /* AND */|| op==0xF4 /* OR */) {
        nextToken();
        if (vErroProc) return;

        level32(hold);
        if (vErroProc) return;

        if (op==0xF3)
            *result = (*result && *hold);
        else
            *result = (*result || *hold);
    }

    return;
}

//--------------------------------------------------------------------------------------
// Process logic conditions
//--------------------------------------------------------------------------------------
void level32(unsigned char *result)
{
    unsigned char  op;
    unsigned char hold[50];
    union uValue resValue;
    union uValue holValue;
    float fValue;
    unsigned char value_type_ant=0;
//if (debugON3) print((char*)".Aquilevel32.");

    level4(result);
    if (vErroProc) return;

    op = *token;
    if (op=='=' || op=='<' || op=='>' || op==0xF5 /* >= */ || op==0xF6 /* <= */|| op==0xF7 /* <> */) {
//        if (op==0xF5 /* >= */ || op==0xF6 /* <= */|| op==0xF7)
//            pointerRunProg++;

        nextToken();
        if (vErroProc) return;

        value_type_ant = value_type;

        level4(hold);
        if (vErroProc) return;

        if ((value_type_ant=='$' && value_type!='$') || (value_type_ant != '$' && value_type == '$'))
        {
            vErroProc = 16;
            return;
        }

        // Se forem diferentes os 2, se for um deles string, da erro, se nao, passa o inteiro para real
        if (value_type != value_type_ant)
        {
            memcpy(&resValue, result, 4);
            memcpy(&holValue, hold, 4);

            if (value_type == '#')
            {
                fValue = (float)resValue.iVal;
                resValue.fVal = fValue;
            }
            else
            {
                fValue = (float)holValue.iVal;
                holValue.fVal = fValue;
                value_type = '#';
            }

            memcpy(result, resValue.vBytes, 4);
            memcpy(hold, holValue.vBytes, 4);
        }

        if (value_type == '$')
            logicalString(op, (char*)result, (char*)hold);
        else if (value_type == '#')
            logicalNumericFloat(op, (char*)result, (char*)hold, 1);
        else
            logicalNumericInt(op, (char*)result, (char*)hold);
    }

    return;
}

//--------------------------------------------------------------------------------------
// Process integer exponent real/int.
//--------------------------------------------------------------------------------------
void level4(unsigned char *result)
{
    unsigned char hold[50];
    char value_type_ant=0;
    union uValue resValue;
    union uValue holValue;
    float fValue;
//if (debugON3) print((char*)".Aquilevel4.");

    level5(result);
    if (vErroProc) return;

    if (*token== '^') {
        if (value_type == '$')
        {
            vErroProc = 16;
            return;
        }

        nextToken();
        if (vErroProc) return;

        value_type_ant = value_type;

        level4(hold);
        if (vErroProc) return;

        if (value_type == '$')
        {
            vErroProc = 16;
            return;
        }

        // Se forem diferentes os 2, se for um deles string, da erro, se nao, passa o inteiro para real
        if (value_type != value_type_ant)
        {
            memcpy(&resValue, result, 4);
            memcpy(&holValue, hold, 4);

            if (value_type == '#')
            {
                fValue = (float)resValue.iVal;
                resValue.fVal = fValue;
            }
            else
            {
                fValue = (float)holValue.iVal;
                holValue.fVal = fValue;
                value_type = '#';
            }

            memcpy(result, resValue.vBytes, 4);
            memcpy(hold, holValue.vBytes, 4);
        }

        if (value_type == '#')
            arithReal('^', (char*)result, (char*)hold);
        else
            arithInt('^', (char*)result, (char*)hold);        
    }

    return;
}

//--------------------------------------------------------------------------------------
// Is a unary + or -.
//--------------------------------------------------------------------------------------
void level5(unsigned char *result)
{
    char  op;
    union uValue uLevel5;
    int ival;
    float fval;
//if (debugON3) print((char*)".Aquilevel5.");

    op = 0;
    if (token_type==DELIMITER && (*token=='+' || *token=='-')) {
        op = *token;
        nextToken();
        if (vErroProc) return;
    }

    level6(result);
    if (vErroProc) return;

    if (op)
    {
        if (value_type == '$')
        {
            vErroProc = 16;
            return;
        }

        memcpy(&uLevel5, result, 4);

        if (value_type == '#')
        {
            if (op == '-')
            {
                fval = uLevel5.fVal * (-1);
                uLevel5.fVal = fval;
            }
        }
        else
        {
            if (op == '-')
            {
                ival = uLevel5.iVal * (-1);
                uLevel5.iVal = ival;
            }
        }

        memcpy(result, uLevel5.vBytes, 4);
    }

    return;
}

//--------------------------------------------------------------------------------------
// Process parenthesized expression real/int/string or function.
//--------------------------------------------------------------------------------------
void level6(unsigned char *result)
{
//if (debugON4) print((char*)".Aquilevel6.");
//if (debugON4) writech(*token < 0x20? *token + 0x30 : *token);
//if (debugON4) writech('.');

    if ((*token == '(') && (token_type == OPENPARENT)) {
        nextToken();
        if (vErroProc) return;

        level2(result);
        if (*token != ')')
        {
            vErroProc = 15;
            return;
        }

        nextToken();
        if (vErroProc) return;
    }
    else
    {
        primitive(result);
        return;
    }

    return;
}

//--------------------------------------------------------------------------------------
// Find value of number or variable.
//--------------------------------------------------------------------------------------
void primitive(unsigned char *result)
{
    int ix;
    union uValue uNumber;
    unsigned char vRet[10];
    unsigned char *vTempPointer;
//if (debugON3) print((char*)".AquilevelP.");
//if (debugON3) writech(token_type + 0x30);
//if (debugON3) writech('.');

    switch(token_type) {
        case VARIABLE:
            if (strlen((char*)token) < 3)
            {
                value_type=VARTYPEDEFAULT;

                if (strlen((char*)token) == 2 && *(token + 1) < 0x30)
                    value_type = *(token + 1);
            }
            else
            {
                value_type = *(token + 2);
            }

            find_var((char*)token, vRet);
            if (vErroProc) return;
            if (value_type == '$')  // Tipo da variavel
                strcpy((char*)result,(char*)vRet);
            else
            {
                for (ix = 0;ix < 5;ix++)
                    result[ix] = vRet[ix];
            }
            nextToken();
            if (vErroProc) return;
            return;
        case QUOTE:
            value_type='$';
            strcpy((char*)result,(char*)token);
            nextToken();
            if (vErroProc) return;
            return;
        case NUMBER:
            if (strchr((char*)token,'.'))  // verifica se eh numero inteiro ou real
            {
                value_type='#'; // Real
                uNumber.fVal = atof((char*)token);
                memcpy(result, uNumber.vBytes, 4);
            }
            else
            {
                value_type='%'; // Inteiro
                uNumber.iVal = atoi((char*)token);
                memcpy(result, uNumber.vBytes, 4);
            }

            nextToken();
            if (vErroProc) return;
            return;
        case COMMAND:
            vTempPointer = (unsigned char*)pointerRunProg;
            *token = *vTempPointer;
            pointerRunProg = pointerRunProg + 1;
            executeToken(*vTempPointer);  // Retorno do resultado da funcao deve voltar pela variavel *token. value_type tera o tipo de retorno
            if (vErroProc) return;

            if (value_type == '$')  // Tipo do retorno
                strcpy((char*)result,(char*)token);
            else
            {
                for (ix = 0; ix < 4; ix++)
                {
                    result[ix] = *(token + ix);
                }
            }

            nextToken();
            if (vErroProc) return;
            return;
        default:
            vErroProc = 14;
            return;
    }

    return;
}

//--------------------------------------------------------------------------------------
// Perform the specified arithmetic int
//--------------------------------------------------------------------------------------
void arithInt(char o, char *r, char *h)
{
    int ex;
    union uValue resValue;
    union uValue holValue;

    memcpy(&resValue, r, 4);
    memcpy(&holValue, h, 4);

    switch(o) {
        case '-':
            resValue.iVal = resValue.iVal - holValue.iVal;
            break;
        case '+':
            resValue.iVal = resValue.iVal + holValue.iVal;
            break;
        case '*':
            resValue.iVal = resValue.iVal * holValue.iVal;
            break;
        case '/':
            resValue.iVal = (resValue.iVal)/(holValue.iVal);
            break;
        case '^':
            ex = resValue.iVal;
            if (holValue.iVal==0) {
                resValue.iVal = 1;
                break;
            }
            ex = powNum(resValue.iVal,holValue.iVal);
            resValue.iVal = ex;
            break;
    }

    memcpy(r, resValue.vBytes, 4);
}

//--------------------------------------------------------------------------------------
// Perform the specified arithmetic real.
//--------------------------------------------------------------------------------------
void arithReal(char o, char *r, char *h)
{
    int ex;
    union uValue resValue;
    union uValue holValue;

    memcpy(&resValue, r, 4);
    memcpy(&holValue, h, 4);

    switch(o) {
        case '-':
            resValue.fVal = resValue.fVal - holValue.fVal;
            break;
        case '+':
            resValue.fVal = resValue.fVal + holValue.fVal;
            break;
        case '*':
            resValue.fVal = resValue.fVal * holValue.fVal;
            break;
        case '/':
            resValue.fVal = (resValue.fVal)/(holValue.fVal);
            break;
        case '^':
            ex = resValue.fVal;
            if (holValue.fVal==0) {
                resValue.fVal = 1;
                break;
            }
            ex = powNum(resValue.fVal,holValue.fVal);
            resValue.fVal = ex;
            break;
    }

    memcpy(r, resValue.vBytes, 4);
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
char logicalNumericFloat(unsigned char o, char *r, char *h, char vTipoRetVar)
{
    union uValue resValue;
    union uValue holValue;

    memcpy(&resValue, r, 4);
    memcpy(&holValue, h, 4);

    value_type = '%';

    switch(o) {
        case '=':
            resValue.iVal = (int)(resValue.fVal == holValue.fVal);
            break;
        case '>':
            resValue.iVal = (int)(resValue.fVal > holValue.fVal);
            break;
        case '<':
            resValue.iVal = (int)(resValue.fVal < holValue.fVal);
            break;
        case 0xF5:
            resValue.iVal = (int)(resValue.fVal >= holValue.fVal);
            break;
        case 0xF6:
            resValue.iVal = (int)(resValue.fVal <= holValue.fVal);
            break;
        case 0xF7:
            resValue.iVal = (int)(resValue.fVal != holValue.fVal);
            break;
    }

    if (vTipoRetVar)
        memcpy(r, resValue.vBytes, 4);    

    return (char)resValue.iVal;
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void logicalNumericInt(unsigned char o, char *r, char *h)
{
    union uValue resValue;
    union uValue holValue;

    memcpy(&resValue, r, 4);
    memcpy(&holValue, h, 4);

    switch(o) {
        case '=':
            resValue.iVal = (resValue.iVal == holValue.iVal);
            break;
        case '>':
            resValue.iVal = (resValue.iVal > holValue.iVal);
            break;
        case '<':
            resValue.iVal = (resValue.iVal < holValue.iVal);
            break;
        case 0xF5:
            resValue.iVal = (resValue.iVal >= holValue.iVal);
            break;
        case 0xF6:
            resValue.iVal = (resValue.iVal <= holValue.iVal);
            break;
        case 0xF7:
            resValue.iVal = (resValue.iVal != holValue.iVal);
            break;
    }

    memcpy(r, resValue.vBytes, 4);    
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void logicalString(unsigned char o, char *r, char *h)
{
    int ex;

    ex = ustrcmp(r,h);
    value_type = '%';

    switch(o) {
        case '=':
            *r = (ex == 0);
            break;
        case '>':
            *r = (ex > 0);
            break;
        case '<':
            *r = (ex < 0);
            break;
        case 0xF5:
            *r = (ex >= 0);
            break;
        case 0xF6:
            *r = (ex <= 0);
            break;
        case 0xF7:
            *r = (ex != 0);
            break;
    }
}

//--------------------------------------------------------------------------------------
// Reverse the sign.
//--------------------------------------------------------------------------------------
void unaryInt(char o, int *r)
{
    if (o=='-')
        *r = -(*r);
}

//--------------------------------------------------------------------------------------
// Reverse the sign.
//--------------------------------------------------------------------------------------
void unaryReal(char o, float *r)
{
    if (o=='-')
        *r = -(*r);
}

//--------------------------------------------------------------------------------------
// Find the value of a variable.
//--------------------------------------------------------------------------------------
void find_var(char *s, unsigned char *vTemp)
{
    vErroProc = 0x00;

    if (!isalphas(*s)){
        vErroProc = 4; // not a variable
        return;
    }

    if (strlen(s) < 3)
    {
        vTemp[0] = *s;
        vTemp[2] = VARTYPEDEFAULT;

        if (strlen(s) == 2 && *(s + 1) < 0x30)
            vTemp[2] = *(s + 1);

        if (strlen(s) == 2 && isalphas(*(s + 1)))
            vTemp[1] = *(s + 1);
        else
            vTemp[1] = 0x00;
    }
    else
    {
        vTemp[0] = *s++;
        vTemp[1] = *s++;
        vTemp[2] = *s;
    }

    if (!findVariable(vTemp))
    {
        vErroProc = 4; // not a variable
        return;
    }

    return;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void forPush(for_stack i)
{
    if (ftos>FOR_NEST)
    {
        vErroProc = 10;
        return;
    }

    forStack[ftos] = i;
    ftos = ftos + 1;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
for_stack forPop(void)
{
    for_stack i;

    ftos = ftos - 1;

    if (ftos<0)
    {
        vErroProc = 11;
        return(forStack[0]);
    }

    i=forStack[ftos];

    return(i);
}

//-----------------------------------------------------------------------------
// GOSUB stack push function.
//-----------------------------------------------------------------------------
void gosubPush(unsigned long i)
{
    if (gtos>SUB_NEST)
    {
        vErroProc = 12;
        return;
    }

    gosubStack[gtos]=i;

    gtos = gtos + 1;
}

//-----------------------------------------------------------------------------
// GOSUB stack pop function.
//-----------------------------------------------------------------------------
unsigned long gosubPop(void)
{
    unsigned long i;

    gtos = gtos - 1;

    if (gtos<0)
    {
        vErroProc = 13;
        return 0;
    }

    i=gosubStack[gtos];

    return i;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
unsigned int powNum(unsigned int pbase, unsigned char pexp)
{
    unsigned int iz, vRes = pbase;

    if (pexp > 0)
    {
        pexp--;

        for(iz = 0; iz < pexp; iz++)
        {
            vRes = vRes * pbase;
        }
    }
    else
        vRes = 1;

    return vRes;
}

//-----------------------------------------------------------------------------
// Criado porque o sprintf com %f esta dando pau, crashando o MCU
//-----------------------------------------------------------------------------
void floatToChar(float x, char *p) 
{
//    char *s = p + 10; // go to end of buffer
    uint16_t decimals;  // variable to store the decimals
    int units;  // variable to store the units (part to left of decimal place)
    if (x < 0) { // take care of negative numbers
        decimals = (int)(x * -100) % 100; // make 1000 for 3 decimals etc.
        units = (int)(-1 * x);
    } else { // positive numbers
        decimals = (int)(x * 100) % 100;
        units = (int)x;
    }

    if (x < 0)
        sprintf(p, "-%d.%d", units, decimals);
    else
        sprintf(p, "%d.%d", units, decimals);
/*    *--s = (decimals % 10) + '0';
    decimals /= 10; // repeat for as many decimal places as you need
    *--s = (decimals % 10) + '0';
    *--s = '.';

    while (units > 0) {
        *--s = (units % 10) + '0';
        units /= 10;
    }
    if (x < 0) *--s = '-'; // unary minus sign for negative numbers*/
}

//-----------------------------------------------------------------------------
// FUNCOES BASIC
//-----------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Text Screen Mode (40 cols x 24 rows)
// Syntaxe:
//          TEXT
//--------------------------------------------------------------------------------------
int basText(void)
{
    fgcolor = TFT_WHITE;
    bgcolor = TFT_BLACK;
    clearScr();
    return 0;
}

//--------------------------------------------------------------------------------------
// DUMB FUNCTIONS
//--------------------------------------------------------------------------------------
int basInverse(void) { return 0;}
int basNormal(void) { return 0;}
int basGr(void) { return 0;}
int basHgr(void) { return 0;}
int basColor(void) { return 0;}
int basPlot(void) { return 0;}
int basHVlin(unsigned char vTipo) { return 0;}   // 1 - HLIN, 2 - VLIN
int basScrn(void) { return 0;}
int basHcolor(void) { return 0;}
int basHplot(void) { return 0;}
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Joga pra tela Texto.
// Syntaxe:
//      Print "<Texto>"/<value>[, "<Texto>"/<value>][; "<Texto>"/<value>]
//-----------------------------------------------------------------------------
int basPrint(void)
{
    unsigned char answer[200];
    union uValue uPrint;
    int len=0, spaces;
    char last_delim;

    do {
        nextToken();
        if (vErroProc) return 0;

        if (tok == EOL || tok == FINISHED)
            break;

        if (token_type == QUOTE) { // is string
            print((char*)token);

            nextToken();
            if (vErroProc) return 0;
        }
        else if (*token!=':') { // is expression
//            last_token_type = token_type;

            putback();

            getExp(answer);
            if (vErroProc) return 0;

            if (value_type != '$')
            {
                memcpy(&uPrint, answer, 4);

                if (value_type == '#')
                {
                    // Real
                    memset(answer, '\0', 20);
                    floatToChar(uPrint.fVal, (char*)answer);
                }
                else
                {
                    // Inteiro
                    sprintf((char*)answer, "%d", uPrint.iVal);
                }
            }

            print((char*)answer);

            nextToken();
            if (vErroProc) return 0;
        }

        last_delim = *token;

        if (*token==',') {
            // compute number of spaces to move to next tab
            spaces = 8 - (len % 8);
            while(spaces) {
                writech(' ');
                spaces--;
            }
        }
        else if (*token==';' || *token=='+')
            /* do nothing */;
        else if (*token==':')
        {
            pointerRunProg = pointerRunProg - 1;
        }
        else if (tok!=EOL && tok!=FINISHED && *token!=':')
        {
            vErroProc = 14;
            return 0;
        }
    } while (*token==';' || *token==',' || *token=='+');
//if (debugON2) sprintf(dataShow, ".Aqui.3.%lX.", pointerRunProg);
//if (debugON2) print(dataShow);

    if (tok == EOL || tok == FINISHED || *token==':') {
        if (last_delim != ';' && last_delim!=',')
            println((char*)"");
    }

    return 0;
}

//-----------------------------------------------------------------------------
// Devolve o caracter ligado ao codigo ascii passado
// Syntaxe:
//      CHR$(<codigo ascii>)
//-----------------------------------------------------------------------------
int basChr(void)
{
    unsigned char answer[10];
    union uValue uChr;
    float fValue;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */

        putback();

        getExp(answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        memcpy(&uChr, answer, 4);

        if (value_type == '#')
        {
            fValue = uChr.fVal;
            uChr.iVal = (int)fValue;
            value_type = '%';
        }

        // Inteiro
        if (uChr.iVal<0 || uChr.iVal>255)
        {
            vErroProc = 5;
            return 0;
        }
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    *token=uChr.vBytes[0];
    *(token + 1)=0x00;
    value_type='$';

    return 0;
}

//-----------------------------------------------------------------------------
// Devolve o numerico da string
// Syntaxe:
//      VAL(<string>)
//-----------------------------------------------------------------------------
int basVal(void)
{
    unsigned char answer[20];
    union uValue uVal;
    int iValue;
    float fValue;
    char last_value_type=' ';

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        if (strchr((char*)token,'.'))  // verifica se eh numero inteiro ou real
        {
            last_value_type='#'; // Real
            fValue=atof((char*)token);
        }
        else
        {
            last_value_type='%'; // Inteiro
            iValue=atoi((char*)token);
        }
    }
    else { /* is expression */

        putback();

        getExp(answer);
        if (vErroProc) return 0;

        if (value_type != '$')
        {
            vErroProc = 16;
            return 0;
        }

        if (strchr((char*)answer,'.'))  // verifica se eh numero inteiro ou real
        {
            last_value_type='#'; // Real
            fValue=atof((char*)answer);
        }
        else
        {
            last_value_type='%'; // Inteiro
            iValue=atoi((char*)answer);
        }
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    if (last_value_type=='#')
        uVal.fVal = fValue;
    else
        uVal.iVal = iValue;

    memcpy(token, uVal.vBytes, 4);

    value_type = last_value_type;

    return 0;
}

//-----------------------------------------------------------------------------
// Devolve a string do numero
// Syntaxe:
//      STR$(<Numero>)
//-----------------------------------------------------------------------------
int basStr(void)
{
    unsigned char answer[50];
    union uValue uStr;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */

        putback();

        getExp(answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    memcpy(&uStr, answer, 4);

    if (value_type=='#')    // real
    {
        floatToChar(uStr.fVal, (char*)token);
    }
    else    // Inteiro
    {
        sprintf((char*)token, "%d", uStr.iVal);
    }

    value_type='$';

    return 0;
}

//-----------------------------------------------------------------------------
// Devolve o tamanho da string
// Syntaxe:
//      LEN(<string>)
//-----------------------------------------------------------------------------
int basLen(void)
{
    unsigned char answer[200];
    union uValue uLen;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
    	uLen.iVal=strlen((char*)token);
    }
    else { /* is expression */

        putback();

        getExp(answer);
        if (vErroProc) return 0;

        if (value_type != '$')
        {
            vErroProc = 16;
            return 0;
        }

        uLen.iVal=strlen((char*)answer);
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    memcpy(token, uLen.vBytes, 4);

    value_type='%';

    return 0;
}

//-----------------------------------------------------------------------------
// Devolve qtd memoria usuario disponivel
// Syntaxe:
//      FRE(0)
//-----------------------------------------------------------------------------
int basFre(void)
{
    unsigned char answer[50];
    union uValue uFre;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(answer);
        if (vErroProc) return 0;

        memcpy(&uFre, answer, 4);

        if (uFre.iVal!=0)
        {
            vErroProc = 5;
            return 0;
        }
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    // Calcula Quantidade de Memoria e printa na tela
    uFre.iVal = (int)((pStartArrayVar - pStartSimpVar) + (pStartString - pStartArrayVar));

    memcpy(token, uFre.vBytes, 4);

    value_type='%';

    return 0;
}

//--------------------------------------------------------------------------------------
// Devolve o codigo ligado ao caracter
// Syntaxe:
//      ASC(<val>)
//--------------------------------------------------------------------------------------
int basAsc(void)
{
    unsigned char answer[20];
    union uValue uAsc;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        if (strlen((char*)token)>1)
        {
            vErroProc = 6;
            return 0;
        }

        memcpy(&uAsc, token, 4);
        uAsc.iVal &= 0x000000FF;
    }
    else { /* is expression */
        putback();

        getExp(answer);
        if (vErroProc) return 0;

        if (value_type != '$')
        {
            vErroProc = 16;
            return 0;
        }

        memcpy(&uAsc, answer, 4);
        uAsc.iVal &= 0x000000FF;
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    memcpy(token, uAsc.vBytes, 4);

    value_type = '%';

    return 0;
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
int basLeftRightMid(char pTipo)
{
    unsigned int ix = 0, iy = 0, iz = 0, iw = 0;
    unsigned char answer[200], vTemp[200];
    unsigned int vqtd = 0, vstart = 0;
    union uValue uLrm;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        strcpy((char*)vTemp, (char*)token);
    }
    else { /* is expression */
        putback();

        getExp(answer);
        if (vErroProc) return 0;

        if (value_type != '$')
        {
            vErroProc = 16;
            return 0;
        }

        strcpy((char*)vTemp, (char*)answer);
    }

    nextToken();
    if (vErroProc) return 0;

    // Deve ser uma virgula para Receber a qtd, e se for mid = a posiao incial
    if (*token!=',')
    {
        vErroProc = 18;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        if (pTipo=='M')
        {
            getExp(answer);
            memcpy(&uLrm, answer, 4);
            if (value_type == '#')
                vstart = (int)uLrm.fVal;
            else
                vstart = uLrm.iVal;
            vqtd=strlen((char*)vTemp);
        }
        else
        {
            getExp(answer);
            memcpy(&uLrm, answer, 4);
            if (value_type == '#')
                vqtd = (int)uLrm.fVal;
            else
                vqtd = uLrm.iVal;
        }

        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }
    }

    if (pTipo == 'M')
    {
        // Deve ser uma virgula para Receber a qtd
        if (*token==',')
        {
            nextToken();
            if (vErroProc) return 0;

            if (token_type == QUOTE) { /* is string, error */
                vErroProc = 16;
                return 0;
            }
            else { /* is expression */
                //putback();

                getExp(answer);

                if (vErroProc) return 0;

                if (value_type == '$')
                {
                    vErroProc = 16;
                    return 0;
                }

                memcpy(&uLrm, answer, 4);
                if (value_type == '#')
                    vqtd = (int)uLrm.fVal;
                else
                    vqtd = uLrm.iVal;
            }
        }
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    if (vqtd > strlen((char*)vTemp))
    {
        if (pTipo=='M')
            vqtd = (strlen((char*)vTemp) - vstart) + 1;
        else
            vqtd = strlen((char*)vTemp);
    }

    if (pTipo == 'L') // Left$
    {
        for (ix = 0; ix < vqtd; ix++)
            *(token + ix) = vTemp[ix];
        *(token + ix) = 0x00;
    }
    else if (pTipo == 'R') // Right$
    {
        iy = strlen((char*)vTemp);
        iz = (iy - vqtd);
        iw = 0;
        for (ix = iz; ix < iy; ix++)
            *(token + iw++) = vTemp[ix];
        *(token + iw)=0x00;
    }
    else  // Mid$
    {
        iy = strlen((char*)vTemp);
        iw=0;
        vstart--;

        for (ix = vstart; ix < iy; ix++)
        {
            if (iw <= iy && vqtd-- > 0)
                *(token + iw++) = vTemp[ix];
            else
                break;
        }

        *(token + iw) = 0x00;
    }

    value_type = '$';

    return 0;
}

//--------------------------------------------------------------------------------------
// IF <cond> THEN <to do> [<else> <to do>]
//--------------------------------------------------------------------------------------
int basIf(void)
{
    unsigned int vCond = 0;
    union uValue uIf;
    unsigned char answer[5];
    unsigned char *vTempPointer;

    getExp(answer); // get target value

    if (value_type == '$' || value_type == '#') {
        vErroProc = 16;
        return 0;
    }

    memcpy(&uIf, answer, 4);
    vCond = uIf.iVal;

    nextToken();
    if (vErroProc) return 0;

    if (*token!=0x83)
    {
        vErroProc = 8;
        return 0;
    }

    if (vCond)
    {
        // Vai pro proximo comando apos o Then e continua
        pointerRunProg = pointerRunProg + 1;

        // simula ":" para continuar a execucao
        doisPontos = 1;
    }
    else
    {
        // Ignora toda a linha
        vTempPointer = (unsigned char*)pointerRunProg;
        while (*vTempPointer)
        {
            pointerRunProg = pointerRunProg + 1;
            vTempPointer = (unsigned char*)pointerRunProg;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Atribuir valor a uma variavel/array - comando opcional.
// Syntaxe:
//            [LET] <variavel/array(x[,y])> = <string/valor>
//--------------------------------------------------------------------------------------
int basLet(void)
{
    long vRetFV;
    unsigned char varTipo;
    unsigned char value[200];
    union uValue uLet;
    unsigned char vArray = 0;
    unsigned char *vTempPointer;
    float fValue;
    int iValue;

    /* get the variable name */
    nextToken();
    if (vErroProc) return 0;

    if (!isalphas(*token)) {
        vErroProc = 4;
        return 0;
    }

    if (strlen((char*)token) < 3)
    {
        varName[0] = *token;
        varTipo = VARTYPEDEFAULT;

        if (strlen((char*)token) == 2 && *(token + 1) < 0x30)
            varTipo = *(token + 1);

        if (strlen((char*)token) == 2 && isalphas(*(token + 1)))
            varName[1] = *(token + 1);
        else
            varName[1] = 0x00;

        varName[2] = varTipo;
    }
    else
    {
        varName[0] = *token;
        varName[1] = *(token + 1);
        varName[2] = *(token + 2);
        varTipo = varName[2];
    }

    // verifica se é array (abre parenteses no inicio)
    vTempPointer = (unsigned char*)pointerRunProg;
    if (*vTempPointer == 0x28)
    {
        vRetFV = findVariable(varName);
        if (vErroProc) return 0;

        if (!vRetFV)
        {
            vErroProc = 4;
            return 0;
        }

        vArray = 1;
    }

    // get the equals sign
    nextToken();
    if (vErroProc) return 0;

    if (*token!='=') {
        vErroProc = 3;
        return 0;
    }
    /* get the value to assign to varName */
    getExp(value);

    memcpy(&uLet, value, 4);

    if (varTipo == '#' && value_type == '%')
    {
        iValue = uLet.iVal;
        uLet.fVal = (float)iValue;
        memcpy(value, uLet.vBytes, 4);
    }
    else if (varTipo == '%' && value_type == '#')
    {
        fValue = uLet.fVal;
        uLet.iVal = (int)fValue;
        memcpy(value, uLet.vBytes, 4);
    }

    // assign the value
    if (!vArray)
    {
        vRetFV = findVariable(varName);
        // Se nao existe a variavel, cria variavel e atribui o valor
        if (!vRetFV)
            createVariable(varName, value, varTipo);
        else // se ja existe, altera
            updateVariable((unsigned long*)(vRetFV + 3), value, varTipo, 1);
    }
    else
    {
        updateVariable((unsigned long*)vRetFV, value, varTipo, 2);
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Salta para uma linha se erro
// Syntaxe:
//          ONERR GOTO <num.linha>
//--------------------------------------------------------------------------------------
int basOnErr(void)
{
    unsigned char* vNextAddrGoto;
    unsigned char sNumLin[5];
    union uValue uOnErr;
    unsigned char *vTempPointer;

    vTempPointer = (unsigned char*)pointerRunProg;

    // Se nao for goto, erro
    if (*vTempPointer != 0x89)
    {

        vErroProc = 14;
        return 0;
    }

    // soma mais um pra ir pro numero da linha
    pointerRunProg = pointerRunProg + 1;

    getExp(sNumLin); // get target value

    if (value_type == '$' || value_type == '#') {
        vErroProc = 17;
        return 0;
    }

    memcpy(&uOnErr, sNumLin, 4);
    vNextAddrGoto = (unsigned char*)findNumberLine((unsigned short)uOnErr.iVal, 0, 0);

    if (vNextAddrGoto > 0)
    {
        if ((unsigned int)(((unsigned int)*(vNextAddrGoto + 4) << 8) | *(vNextAddrGoto + 5)) == (unsigned int)uOnErr.iVal)
        {
            onErrGoto = (unsigned long)vNextAddrGoto;
            return 0;
        }
        else
        {
            vErroProc = 7;
            return 0;
        }
    }
    else
    {
        vErroProc = 7;
        return 0;
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Salta para uma linha, sem retorno
// Syntaxe:
//          GOTO <num.linha>
//--------------------------------------------------------------------------------------
int basGoto(void)
{
    unsigned char* vNextAddrGoto;
    unsigned char sNumLin[5];
    union uValue uGoto;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED)
    {
        vErroProc = 14;
        return 0;
    }

    putback();

    getExp(sNumLin); // get target value

    if (value_type == '$' || value_type == '#') {
        vErroProc = 17;
        return 0;
    }

    memcpy(&uGoto, sNumLin, 4);
    vNextAddrGoto = (unsigned char*)findNumberLine((unsigned short)uGoto.iVal, 0, 0);

    if (vNextAddrGoto > 0)
    {
        if ((unsigned int)(((unsigned int)*(vNextAddrGoto + 4) << 8) | *(vNextAddrGoto + 5)) == (unsigned int)uGoto.iVal)
        {
            changedPointer = (unsigned long)vNextAddrGoto;
            return 0;
        }
        else
        {
            vErroProc = 7;
            return 0;
        }
    }
    else
    {
        vErroProc = 7;
        return 0;
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Salta para uma linha e guarda a posicao atual para voltar
// Syntaxe:
//          GOSUB <num.linha>
//--------------------------------------------------------------------------------------
int basGosub(void)
{
    unsigned char* vNextAddrGoto;
    unsigned char sNumLin[5];
    union uValue uGosub;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED)
    {
        vErroProc = 14;
        return 0;
    }

    putback();

    getExp(sNumLin); // get target valuedel 20

    if (value_type == '$' || value_type == '#') {
        vErroProc = 17;
        return 0;
    }
    
    memcpy(&uGosub, sNumLin, 4);
    vNextAddrGoto = (unsigned char*)findNumberLine((unsigned short)uGosub.iVal, 0, 0);

    if (vNextAddrGoto > 0)
    {
        if ((unsigned int)(((unsigned int)*(vNextAddrGoto + 4) << 8) | *(vNextAddrGoto + 5)) == (unsigned int)uGosub.iVal)
        {
            gosubPush(nextAddr);
            changedPointer = (unsigned long)vNextAddrGoto;
            return 0;
        }
        else
        {
            vErroProc = 7;
            return 0;
        }
    }
    else
    {
        vErroProc = 7;
        return 0;
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Retorna de um Gosub
// Syntaxe:
//          RETURN
//--------------------------------------------------------------------------------------
int basReturn(void)
{
    unsigned long i;

    i = gosubPop();

    changedPointer = i;

    return 0;
}

//--------------------------------------------------------------------------------------
// Retorna um numero real como inteiro
// Syntaxe:
//          INT(<number real>)
//--------------------------------------------------------------------------------------
int basInt(void)
{
    union uValue uInt;
    unsigned char sReal[10];
    float fValue;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    putback();

    getExp(sReal); //

    if (value_type == '$')
    {
        vErroProc = 16;
        return 0;
    }

    memcpy(&uInt, sReal, 4);

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    if (value_type == '#')
    {
        fValue = uInt.fVal;
        uInt.iVal = (int)fValue;
    }

    value_type='%';

    memcpy(token, uInt.vBytes, 4);

    return 0;
}

//--------------------------------------------------------------------------------------
// Retorna um numero absoluto como inteiro
// Syntaxe:
//          ABS(<number real>)
//--------------------------------------------------------------------------------------
int basAbs(void)
{
    union uValue uAbs;
    unsigned char sReal[10];
    float fValue;
    int iValue;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    putback();

    getExp(sReal); //

    if (value_type == '$')
    {
        vErroProc = 16;
        return 0;
    }

    memcpy(&uAbs, sReal, 4);

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    if (value_type == '#')
    {
        fValue = uAbs.fVal;
        if (fValue < 1)
            fValue *= (-1.0);
        uAbs.fVal = fValue;
    }   
    else
    {
        iValue = abs(uAbs.iVal);
        uAbs.iVal = iValue;
    }

    memcpy(token, uAbs.vBytes, 4);

    return 0;
}

//--------------------------------------------------------------------------------------
// Retorna um numero randomicamente
// Syntaxe:
//          RND(<number>)
//--------------------------------------------------------------------------------------
int basRnd(void)
{
    int vReal, iRand;
    union uValue uRand;
    float vRand;
    unsigned char sReal[10];

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    putback();

    getExp(sReal); //

    if (value_type == '$')
    {
        vErroProc = 16;
        return 0;
    }

    memcpy(&uRand, sReal, 4);
    vReal = uRand.iVal;

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type != CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    if (vReal == 0)
    {
        vRand = (float)randSeed / 100.00f;
    }
    else if (vReal < 0)
    {
#ifdef __USE_CPP_RAND_FUNC__
        srand(HAL_GetTick());
        vRand = (float)rand() / (float)RAND_MAX;
        randSeed = vRand;
#else
        randSeed = HAL_GetTick();
        iRand = randSeed * 1664525UL + 1013904223UL;
        iRand = iRand % 101;
        vRand = (float)iRand / 100.00f;
        randSeed = iRand;
#endif
    }
    else if (vReal > 0)
    {
#ifdef __USE_CPP_RAND_FUNC__
        vRand = (float)rand() / (float)RAND_MAX;
        randSeed = vRand;
#else
        iRand = randSeed * 1664525UL + 1013904223UL;
        iRand = iRand % 101;
        vRand = (float)iRand / 100.00f;
        randSeed = iRand;
#endif
    }
    else
    {
        vErroProc = 5;
        return 0;
    }

    uRand.fVal = vRand;

    value_type='#';

    memcpy(token, uRand.vBytes, 4);

    return 0;
}

//--------------------------------------------------------------------------------------
// Seta posicao vertical (linha em texto e y em grafico)
// Syntaxe:
//          VTAB <numero>
//--------------------------------------------------------------------------------------
int basVtab(void)
{
    unsigned char sRow[10];
    union uValue uVtab;
    float fValue;

    getExp(sRow);

    if (value_type == '$') {
        vErroProc = 16;
        return 0;
    }

    memcpy(&uVtab, sRow, 4);

    if (value_type == '#')
    {
        fValue = uVtab.fVal;
        uVtab.iVal = (int)fValue;
        value_type = '%';
    }

    outSetCursor(outGetPosX(), (uVtab.iVal * (int)TFT_FATOR_ROW));

    return 0;
}

//--------------------------------------------------------------------------------------
// Seta posicao horizontal (coluna em texto e x em grafico)
// Syntaxe:
//          HTAB <numero>
//--------------------------------------------------------------------------------------
int basHtab(void)
{
    unsigned char sCol[10];
    union uValue uHtab;
    float fValue;

    getExp(sCol);

    if (value_type == '$') {
        vErroProc = 16;
        return 0;
    }

    memcpy(&uHtab, sCol, 4);

    if (value_type == '#')
    {
        fValue = uHtab.fVal;
        uHtab.iVal = (int)fValue;
        value_type = '%';
    }

    outSetCursor((uHtab.iVal * (int)TFT_FATOR_COL), outGetPosY());

    return 0;
}

//--------------------------------------------------------------------------------------
// Finaliza o programa sem erro
// Syntaxe:
//          END
//--------------------------------------------------------------------------------------
int basEnd(void)
{
    nextAddr = 0;

    return 0;
}

//--------------------------------------------------------------------------------------
// Finaliza o programa com erro
// Syntaxe:
//          STOP
//--------------------------------------------------------------------------------------
int basStop(void)
{
    vErroProc = 1;

    return 0;
}

//--------------------------------------------------------------------------------------
// Retorna 'n' Espaços
// Syntaxe:
//          SPC <numero>
//--------------------------------------------------------------------------------------
int basSpc(void)
{
    union uValue uSpc;
    int ix = 0;
    unsigned char answer[20];
    float fValue;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        memcpy(&uSpc, answer, 4);

        if (value_type == '#')
        {
            fValue = uSpc.fVal;
            uSpc.iVal = (int)fValue;
            value_type = '%';
        }
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    for (ix = 0; ix < uSpc.iVal; ix++)
        *(token + ix) = ' ';

    *(token + ix) = 0;
    value_type = '$';

    return 0;
}

//--------------------------------------------------------------------------------------
// Advance 'n' columns
// Syntaxe:
//          TAB <numero>
//--------------------------------------------------------------------------------------
int basTab(void)
{
    unsigned char answer[20];
    union uValue uTab;
    int vTab, vColumn, vRow;
    float fValue;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        memcpy(&uTab, answer, 4);

        if (value_type == '#')
        {
            fValue = uTab.fVal;
            uTab.iVal = (int)fValue;
            value_type = '%';
        }
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    vTab=uTab.iVal;

    vColumn = outGetPosX();
    vRow = outGetPosY();

    if (vTab>vColumn)
    {
        vColumn = vColumn + vTab;

        while (vColumn > out_width)
        {
            vColumn = vColumn - out_width;
            if (outGetPosY() < out_height)
                vRow += TFT_FATOR_ROW;
        }

        outSetCursor(vColumn, vRow);
    }

    *token = ' ';
    value_type='$';

    return 0;
}

//--------------------------------------------------------------------------------------
int forFind(for_stack *i, unsigned char* endLastVar)
{
    int ix;

    for(ix = 0; ix < ftos; ix++)
    {
        if (forStack[ix].nameVar[0] == endLastVar[1] && forStack[ix].nameVar[1] == endLastVar[2])
        {
            *i = forStack[ix];

            return ix;
        }
        else if (!forStack[ix].nameVar[0])
            return -1;
    }

    return -1;
}

//--------------------------------------------------------------------------------------
// Inicio do laco de repeticao
// Syntaxe:
//          FOR <variavel> = <inicio> TO <final> [STEP <passo>] : A variavel sera criada se nao existir
//--------------------------------------------------------------------------------------
int basFor(void)
{
    for_stack i, *j;
    long *endVarCont;
    unsigned char answer[20];
    int iValue = 0;
    unsigned char* endLastVar;
    int vRetVar = -1;
    unsigned char *vTempPointer;
    char vResLog1 = 0, vResLog2 = 0;
    char vResLog3 = 0, vResLog4 = 0;

    basLet();
    if (vErroProc) return 0;

    endLastVar = (unsigned char*)(atuVarAddr - 3);
    endVarCont = (long*)(atuVarAddr + 1);

    vRetVar = forFind(&i, endLastVar);

    if (vRetVar < 0)
    {
        i.nameVar[0]=endLastVar[1];
        i.nameVar[1]=endLastVar[2];
        i.nameVar[2]=endLastVar[0];
    }

    i.endVar = (long)endVarCont;

    nextToken();
    if (vErroProc) return 0;

    if (tok!=0x86) /* read and discard the TO */
    {
        vErroProc = 9;
        return 0;
    }

    pointerRunProg = pointerRunProg + 1;

    getExp(answer); /* get target value */
    memcpy(&i.target,answer,4);

    if (i.nameVar[2] == '#' && value_type == '%')
    {
        iValue = i.target.iVal;
        i.target.fVal = (float)iValue;
    }

    if (tok==0x88) /* read STEP */
    {
        pointerRunProg = pointerRunProg + 1;

        getExp(answer); /* get target value */
        memcpy(&i.step,answer,4);

        if (i.nameVar[2] == '#' && value_type == '%')
        {
            iValue = i.step.iVal;
            i.step.fVal = (float)iValue;
        }
    }
    else
    {
        i.step.iVal = 1;
        
		iValue = i.step.iVal;
		i.step.fVal = (float)iValue;
    }
        
    endVarCont=(long*)i.endVar;

    // if loop can execute at least once, push info on stack     //    if ((i.step > 0 && *endVarCont <= i.target) || (i.step < 0 && *endVarCont >= i.target))
    if (i.nameVar[2] == '#')
    {
        answer[0] = (unsigned char)((*endVarCont >> 24) & 0x000000FF);
        answer[1] = (unsigned char)((*endVarCont >> 16) & 0x000000FF);
        answer[2] = (unsigned char)((*endVarCont >> 8) & 0x000000FF);
        answer[3] = (unsigned char)(*endVarCont & 0x000000FF);

        vResLog1 = logicalNumericFloat(0xF6 /* <= */, (char*)answer, (char*)i.target.vBytes, 0);
        vResLog2 = logicalNumericFloat(0xF5 /* >= */, (char*)answer, (char*)i.target.vBytes, 0);
        vResLog3 = logicalNumericFloat('>', (char*)i.step.vBytes, 0, 0);
        vResLog4 = logicalNumericFloat('<', (char*)i.step.vBytes, 0, 0);
    }
    else
    {
        vResLog1 = (*endVarCont <= i.target.iVal);
        vResLog2 = (*endVarCont >= i.target.iVal);
        vResLog3 = (i.step.iVal > 0);
        vResLog4 = (i.step.iVal < 0);
    }

    if ((vResLog3 && vResLog1) || (vResLog4 && vResLog2))
    {
        vTempPointer = (unsigned char*)pointerRunProg;
        if (*vTempPointer==0x3A) // ":"
        {
            i.progPosPointerRet = pointerRunProg;
        }
        else
            i.progPosPointerRet = nextAddr;

        if (vRetVar < 0)
            forPush(i);
        else
        {
            j = (forStack + vRetVar);
            j->target = i.target;
            j->step = i.step;
            j->endVar = i.endVar;
            j->progPosPointerRet = i.progPosPointerRet;
        }
    }
    else  /* otherwise, skip loop code alltogether */
    {
        vTempPointer = (unsigned char*)pointerRunProg;
        while(*vTempPointer != 0x87) // Search NEXT
        {
            pointerRunProg = pointerRunProg + 1;
            vTempPointer = (unsigned char*)pointerRunProg;

            // Verifica se chegou no next
            if (*vTempPointer == 0x87)
            {
                // Verifica se tem letra, se nao tiver, usa ele
                if (*(vTempPointer + 1)!=0x00)
                {
                    // verifica se é a mesma variavel que ele tem
                    if (*(vTempPointer + 1) != i.nameVar[0])
                    {
                        pointerRunProg = pointerRunProg + 1;
                        vTempPointer = (unsigned char*)pointerRunProg;
                    }
                    else
                    {
                        if (*(vTempPointer + 2) != i.nameVar[1] && *(vTempPointer + 2) != i.nameVar[2])
                        {
                            pointerRunProg = pointerRunProg + 1;
                            vTempPointer = (unsigned char*)pointerRunProg;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Final/Incremento do Laco de repeticao, voltando para o commando/linha após o FOR
// Syntaxe:
//          NEXT [<variavel>]
//--------------------------------------------------------------------------------------
int basNext(void)
{
    for_stack i;
    int *endVarContI;
    float *endVarContF;
    union uValue uNext;
    unsigned char answer[10];
    char vRetVar = -1;
    unsigned char *vTempPointer;
    char vResLog1 = 0, vResLog2 = 0;
    char vResLog3 = 0, vResLog4 = 0;

    vTempPointer = (unsigned char*)pointerRunProg;
    if (isalphas(*vTempPointer))
    {
        // procura pela variavel no forStack
        nextToken();
        if (vErroProc) return 0;

        if (token_type != VARIABLE)
        {
            vErroProc = 4;
            return 0;
        }

        answer[1] = *token;

        if (strlen((char*)token) == 1)
        {
            answer[0] = 0x00;
            answer[2] = 0x00;
        }
        else if (strlen((char*)token) == 2)
        {
            if (*(token + 1) < 0x30)
            {
                answer[0] = *(token + 1);
                answer[2] = 0x00;
            }
            else
            {
                answer[0] = 0x00;
                answer[2] = *(token + 1);
            }
        }
        else
        {
            answer[0] = *(token + 2);
            answer[2] = *(token + 1);
        }

        vRetVar = forFind(&i,answer);

        if (vRetVar < 0)
        {
            vErroProc = 11;
            return 0;
        }
    }
    else // faz o pop da pilha
        i = forPop(); // read the loop info


    if (i.nameVar[2] == '#')
    {
        endVarContF = (float*)i.endVar;
        *endVarContF = *endVarContF + i.step.fVal; // inc/dec, using step, control variable
        uNext.fVal = *endVarContF; // inc/dec, using step, control variable
    }
    else
    {
        endVarContI = (int*)i.endVar;
        *endVarContI = *endVarContI + i.step.iVal; // inc/dec, using step, control variable
        uNext.iVal = *endVarContI; // inc/dec, using step, control variable
    }

    if (i.nameVar[2] == '#')
    {
        vResLog1 = logicalNumericFloat('>', (char*)uNext.vBytes, (char*)i.target.vBytes, 0);
        vResLog2 = logicalNumericFloat('<', (char*)uNext.vBytes, (char*)i.target.vBytes, 0);
        vResLog3 = logicalNumericFloat('>', (char*)i.step.vBytes, 0, 0);
        vResLog4 = logicalNumericFloat('<', (char*)i.step.vBytes, 0, 0);
    }
    else
    {
        vResLog1 = (uNext.iVal > i.target.iVal);
        vResLog2 = (uNext.iVal < i.target.iVal);
        vResLog3 = (i.step.iVal > 0);
        vResLog4 = (i.step.iVal < 0);
    }


    // compara se ja chegou no final  //     if ((i.step > 0 && *endVarCont>i.target) || (i.step < 0 && *endVarCont<i.target))
    if ((vResLog3 && vResLog1) || (vResLog4 && vResLog2))
        return 0 ;  // all done

    changedPointer = i.progPosPointerRet;  // loop

    if (vRetVar < 0)
        forPush(i);  // otherwise, restore the info

    return 0;
}

//--------------------------------------------------------------------------------------
//  Funcoes Trigonometricas Sin/Cos/Tan = Radians. Log/Exp/Sqrt = Value
//      <funcao>(<radians/value>)
//--------------------------------------------------------------------------------------
int basTrig(unsigned char pFunc)
{
    int iReal = 0;
    float vReal;
    union uValue uTrig;
    unsigned char sReal[10];

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    putback();

    getExp(sReal); //

    if (value_type == '$')
    {
        vErroProc = 16;
        return 0;
    }

    memcpy(&uTrig, sReal, 4);

    if (value_type != '#')
    {
        value_type='#'; // Real
        iReal = uTrig.iVal;
        uTrig.fVal = (float)iReal;
        vReal = uTrig.fVal;
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    switch (pFunc)
    {
        case 1: // sin
            uTrig.fVal = sin(vReal);
            break;
        case 2: // cos
            uTrig.fVal = cos(vReal);
            break;
        case 3: // tan
            uTrig.fVal = tan(vReal);
            break;
        case 4: // log (ln)
            uTrig.fVal = log(vReal);
            break;
        case 5: // exp
            uTrig.fVal = exp(vReal);
            break;
        case 6: // sqrt
            uTrig.fVal = sqrt(vReal);
            break;
        default:
            vErroProc = 14;
            return 0;
    }

    memcpy(token, uTrig.vBytes, 4);

    value_type = '#';

    return 0;
}

//--------------------------------------------------------------------------------------
//  Comandos de memoria
//      Leitura de Memoria:   peek(<endereco>)
//      Gravacao em endereco: poke(<endereco>,<byte>)
//--------------------------------------------------------------------------------------
int basPeekPoke(char pTipo)
{
    unsigned char sEnd[20], sByte[5];
    union uValue uEnd;
    union uValue uByte;
    unsigned char *vEnd = 0;
    unsigned int vByte = 0;

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(sEnd);

        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        memcpy(&uEnd, sEnd, 4);
    }

    // Deve ser uma virgula para Receber a qtd
    if (pTipo == 'W')
    {
        if (*token==',')
        {
            nextToken();
            if (vErroProc) return 0;

            if (token_type == QUOTE) { /* is string, error */
                vErroProc = 16;
                return 0;
            }
            else { /* is expression */
                //putback();

                getExp(sByte);

                if (vErroProc) return 0;

                if (value_type == '$')
                {
                    vErroProc = 16;
                    return 0;
                }

                memcpy(&uByte, sByte, 4);
            }
        }
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    if (pTipo == 'R')
    {
        vEnd = (unsigned char*)uEnd.iVal;
        uByte.iVal = *vEnd;
        memcpy(token, uByte.vBytes, 4);
    }
    else
    {
        vEnd = (unsigned char*)uEnd.iVal;
        *vEnd = (char)vByte;
    }

    value_type = '%';

    return 0;
}

//--------------------------------------------------------------------------------------
// Salta para uma linha se VAR
// Syntaxe:
//          ON <VAR> GOSUB <num.linha 1>,<num.linha 2>,...,,<num.linha n>
//          ON <VAR> GOTO <num.linha 1>,<num.linha 2>,...,<num.linha n>
//--------------------------------------------------------------------------------------
int basOnVar(void)
{
    unsigned char* vNextAddrGoto;
    unsigned int vNumLin = 0;
    unsigned char *vTempPointer;
    unsigned int vSalto;
    union uValue uSalto, uLinha;
    unsigned char answer[10];
    int ix;

    vTempPointer = (unsigned char*)pointerRunProg;
    if (isalphas(*vTempPointer))
    {
        // procura pela variavel
        nextToken();
        if (vErroProc) return 0;

        if (token_type != VARIABLE)
        {
            vErroProc = 4;
            return 0;
        }

        putback();

        getExp(answer);
        if (vErroProc) return 0;

        if (value_type != '%')
        {
            vErroProc = 16;
            return 0;
        }

        memcpy(&uSalto, answer, 4);
        if (uSalto.iVal == 0 || uSalto.iVal > 255)
        {
            vErroProc = 5;
            return 0;
        }
    }
    else
    {
        vErroProc = 4;
        return 0;
    }

    vTempPointer = (unsigned char*)pointerRunProg;

    // Se nao for goto ou gosub, erro
    if (*vTempPointer != 0x89 && *vTempPointer != 0x8A)
    {
        vErroProc = 14;
        return 0;
    }

    vSalto = *vTempPointer;
    ix = 0;
    pointerRunProg = pointerRunProg + 1;

    while (1)
    {
        getExp(answer); // get target value

        if (value_type == '$' || value_type == '#') {
            vErroProc = 16;
            return 0;
        }

        ix++;

        if (ix == uSalto.iVal)
            break;

        nextToken();
        if (vErroProc) return 0;

        // Deve ser uma virgula
        if (*token!=',')
        {
            vErroProc = 18;
            return 0;
        }

        nextToken();
        if (vErroProc) return 0;

        putback();
    }

    if (ix == 0 || ix > uSalto.iVal)
    {
        vErroProc = 14;
        return 0;
    }

    memcpy(&uLinha, answer, 4);
    vNumLin = uLinha.iVal;
    vNextAddrGoto = (unsigned char*)findNumberLine(vNumLin, 0, 0);

    if (vSalto == 0x89)
    {
        // GOTO
        if (vNextAddrGoto > 0)
        {
            if ((unsigned int)(((unsigned int)*(vNextAddrGoto + 4) << 8) | *(vNextAddrGoto + 5)) == vNumLin)
            {
                changedPointer = (unsigned long)vNextAddrGoto;
                return 0;
            }
            else
            {
                vErroProc = 7;
                return 0;
            }
        }
        else
        {
            vErroProc = 7;
            return 0;
        }
    }
    else
    {
        // GOSUB
        if (vNextAddrGoto > 0)
        {
            if ((unsigned int)(((unsigned int)*(vNextAddrGoto + 4) << 8) | *(vNextAddrGoto + 5)) == vNumLin)
            {
                gosubPush(nextAddr);
                changedPointer = (unsigned long)vNextAddrGoto;
                return 0;
            }
            else
            {
                vErroProc = 7;
                return 0;
            }
        }
        else
        {
            vErroProc = 7;
            return 0;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Ler dados no comando DATA
// Syntaxe:
//          READ <variavel>
//--------------------------------------------------------------------------------------
int basRead(void)
{
    unsigned char answer[100];
    union uValue uData;
    int iValue;
    float fValue;
    unsigned char varTipo, vArray = 0;
    unsigned long vTemp;
    unsigned char *vTempLine;
    long vRetFV;
    unsigned char *vTempPointer;

    // Pega a variavel
    nextToken();
    if (vErroProc) return 0;

    if (tok == EOL || tok == FINISHED)
    {
        vErroProc = 4;
        return 0;
    }

    if (token_type == QUOTE) { /* is string */
        vErroProc = 4;
        return 0;
    }
    else { /* is expression */
        // Verifica se comeca com letra, pois tem que ser uma variavel
        if (!isalphas(*token))
        {
            vErroProc = 4;
            return 0;
        }

        if (strlen((char*)token) < 3)
        {
            varName[0] = *token;
            varTipo = VARTYPEDEFAULT;

            if (strlen((char*)token) == 2 && *(token + 1) < 0x30)
                varTipo = *(token + 1);

            if (strlen((char*)token) == 2 && isalphas(*(token + 1)))
                varName[1] = *(token + 1);
            else
                varName[1] = 0x00;

            varName[2] = varTipo;
        }
        else
        {
            varName[0] = *token;
            varName[1] = *(token + 1);
            varName[2] = *(token + 2);
            varTipo = varName[2];
        }
    }

    // Procurar Data
    if (*vDataPointer == 0)
    {
        // Primeira Leitura, procura primeira ocorrencia
        vDataLineAtu = addrFirstLineNumber;

        do
        {
            *vDataPointer = vDataLineAtu;

            vTempLine = (unsigned char*)*vDataPointer;
            if (*(vTempLine + 6) == 0x98)    // Token do comando DATA é o primeiro comando da linha
            {
                *vDataPointer = (vDataLineAtu + 7);
                vDataFirst = vDataLineAtu;
                break;
            }

            vTempLine = (unsigned char*)vDataLineAtu;
            vTemp  = ((*vTempLine & 0xFF) << 24);
            vTemp |= ((*(vTempLine + 1) & 0xFF) << 16);
            vTemp |= ((*(vTempLine + 2) & 0xFF) << 8);
            vTemp |= (*(vTempLine + 3) & 0xFF);

            vDataLineAtu = vTemp;
            vTempLine = (unsigned char*)vDataLineAtu;

        } while (*vTempLine);
    }

    if (*vDataPointer == 0xFFFFFFFF)
    {
        vErroProc = 26;
        return 0;
    }

    *vDataBkpPointerProg = pointerRunProg;

    pointerRunProg = *vDataPointer;

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) {
        strcpy((char*)answer,(char*)token);
        value_type = '$';
    }
    else { /* is expression */
        putback();

        getExp(answer);
        if (vErroProc) return 0;
    }

    // Pega ponteiro atual (proximo numero/char)
    *vDataPointer = pointerRunProg + 1;

    // Devolve ponteiro anterior
    pointerRunProg = *vDataBkpPointerProg;

    // Se nao foi virgula, é final de linha, procura proximo comando data
    if (*token != ',')
    {
        do
        {
            vTempLine = (unsigned char*)vDataLineAtu;
            vTemp  = ((*(vTempLine) & 0xFF) << 24);
            vTemp |= ((*(vTempLine + 1) & 0xFF) << 16);
            vTemp |= ((*(vTempLine + 2) & 0xFF) << 8);
            vTemp |= (*(vTempLine + 3) & 0xFF);

            vDataLineAtu = vTemp;
            vTempLine = (unsigned char*)vDataLineAtu;
            if (!vDataLineAtu)
            {
                *vDataPointer = 0xFFFFFFFF;
                break;
            }

            *vDataPointer = vDataLineAtu;

            vTempLine = (unsigned char*)*vDataPointer;
            if (*(vTempLine + 6) == 0x98)    // Token do comando DATA é o primeiro comando da linha
            {
                *vDataPointer = (vDataLineAtu + 7);
                break;
            }

            vTempLine = (unsigned char*)vDataLineAtu;
        } while (*vTempLine);
    }

    memcpy(&uData, answer, 4);

    if (varTipo != value_type)
    {
        if (value_type == '$' || varTipo == '$')
        {
            vErroProc = 16;
            return 0;
        }

        if (value_type == '%')
        {
            iValue = uData.iVal;
            uData.fVal = (float)iValue;
        }
        else
        {
            fValue = uData.fVal;
            uData.iVal = (int)fValue;
        }

        value_type = varTipo;
    }

    vTempPointer = (unsigned char*)pointerRunProg;
    if (*vTempPointer == 0x28)
    {
        vRetFV = findVariable(varName);
        if (vErroProc) return 0;

        if (!vRetFV)
        {
            vErroProc = 4;
            return 0;
        }

        vArray = 1;
    }

    if (!vArray)
    {
        // assign the value
        vRetFV = findVariable(varName);

        // Se nao existe variavel e inicio sentenca, cria variavel e atribui o valor
        if (!vRetFV)
            createVariable(varName, answer, varTipo);
        else // se ja existe, altera
            updateVariable((unsigned long*)(vRetFV + 3), answer, varTipo, 1);
    }
    else
    {
        updateVariable((unsigned long*)vRetFV, answer, varTipo, 2);
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Volta ponteiro do READ para o primeiro item dos comandos DATA
// Syntaxe:
//          RESTORE
//--------------------------------------------------------------------------------------
int basRestore(void)
{
    vDataLineAtu = vDataFirst;
    *vDataPointer = (vDataLineAtu + 6);

    return 0;
}

//--------------------------------------------------------------------------------------
//  Array (min 1 dimensoes)
//      Sintaxe:
//              DIM <variavel>(<dim 1>[,<dim 2>[,<dim 3>,<dim 4>,...,<dim n>])
//--------------------------------------------------------------------------------------
int basDim(void)
{
    unsigned int vDim[88], ixDim = 0;
    float fValue;
    unsigned char sTempDim[5];
    unsigned char varTipo;
    long vRetFV;
    union uValue uDim;

    nextToken();
    if (vErroProc) return 0;

    // Pega o nome da variavel
    if (!isalphas(*token)) {
        vErroProc = 4;
        return 0;
    }

    if (strlen((char*)token) < 3)
    {
        varName[0] = *token;
        varTipo = VARTYPEDEFAULT;

        if (strlen((char*)token) == 2 && *(token + 1) < 0x30)
            varTipo = *(token + 1);

        if (strlen((char*)token) == 2 && isalphas(*(token + 1)))
            varName[1] = *(token + 1);
        else
            varName[1] = 0x00;

        varName[2] = varTipo;
    }
    else
    {
        varName[0] = *token;
        varName[1] = *(token + 1);
        varName[2] = *(token + 2);
        varTipo = varName[2];
    }

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter depois da variavel, deve ser abre parenteses
    if (tok == EOL || tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    do
    {
        nextToken();
        if (vErroProc) return 0;

        if (token_type == QUOTE) { /* is string, error */
            vErroProc = 16;
            return 0;
        }
        else { /* is expression */
            putback();

            getExp(sTempDim);
            if (vErroProc) return 0;

            if (value_type == '$')
            {
                vErroProc = 16;
                return 0;
            }

            memcpy(&uDim, sTempDim, 4);

            if (value_type == '#')
            {
                fValue = uDim.fVal;
                uDim.iVal = (int)fValue;
                value_type = '%';
            }

            uDim.iVal += 1; // porque nao é de 1 a x, é de 0 a x, entao é x + 1
            vDim[ixDim] = uDim.iVal;

            ixDim++;
        }

        if (*token == ',')
        {
            pointerRunProg = pointerRunProg + 1;
        }
        else
            break;
    } while(1);

    // Deve ter pelo menos 1 elemento
    if (ixDim < 1)
    {
        vErroProc = 21;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    // assign the value
    vRetFV = findVariable(varName);
    // Se nao existe a variavel, cria variavel e atribui o valor
    if (!vRetFV)
        createVariableArray(varName, varTipo, ixDim, vDim);
    else
    {
        vErroProc = 23;
        return 0;
    }

    return 0;
}

//--------------------------------------------------------------------------------------
// Entrada pelo teclado de numeros/caracteres ateh teclar ENTER (INPUT)
// Entrada pelo teclado de um unico caracter ou numero (GET)
// Entrada dos dados de acordo com o tipo de variavel $(qquer), %(Nums), #(Nums & '.')
// Syntaxe:
//          INPUT ["texto",]<variavel> : A variavel sera criada se nao existir
//          GET <variavel> : A variavel sera criada se nao existir
//--------------------------------------------------------------------------------------
int basInputGet(unsigned char pSize)
{
    int ix = 0;
    unsigned char answer[pSize];
    char vtec;
    union uValue uInput;
    char vTemTexto = 0;
    char buffptr[pSize];
    long vRetFV;
    unsigned char varTipo;
    char vArray = 0;
    unsigned char *vTempPointer;

    do {
        nextToken();
        if (vErroProc) return 0;

        if (tok == EOL || tok == FINISHED)
            break;

        if (token_type == QUOTE) /* is string */
        {
            if (vTemTexto)
            {
                vErroProc = 14;
                return 0;
            }

            print((char*)token);

            nextToken();
            if (vErroProc) return 0;

            vTemTexto = 1;
        }
        else /* is expression */
        {
            // Verifica se comeca com letra, pois tem que ser uma variavel agora
            if (!isalphas(*token))
            {
                vErroProc = 4;
                return 0;
            }

            if (strlen((char*)token) < 3)
            {
                varName[0] = *token;
                varTipo = VARTYPEDEFAULT;

                if (strlen((char*)token) == 2 && *(token + 1) < 0x30)
                    varTipo = *(token + 1);

                if (strlen((char*)token) == 2 && isalphas(*(token + 1)))
                    varName[1] = *(token + 1);
                else
                    varName[1] = 0x00;

                varName[2] = varTipo;
            }
            else
            {
                varName[0] = *token;
                varName[1] = *(token + 1);
                varName[2] = *(token + 2);
                varTipo = varName[2];
            }

            answer[0] = 0x00;

            if (pSize == 1)
            {
                // GET
                for (ix = 0; ix < 500; ix++)
                {
                    vtec = readChar(KEY_NOWAIT);
                    if (vtec)
                        break;
                }

                if (varTipo != '$' && vtec)
                {
                    if (!isdigitus(vtec))
                        vtec = 0;
                }

                answer[0] = vtec;
                answer[1] = 0x00;
            }
            else
            {
                // INPUT
                vtec = readStr(pSize, varTipo, buffptr);

                if (buffptr[0] != 0x00 && (vtec == 0x0D || vtec == 0x0A))
                {
                    ix = 0;

                    while (buffptr[ix])
                    {
                        answer[ix] = buffptr[ix];
                        ix++;
                        answer[ix] = 0x00;
                    }
                }

                println((char*)"");
            }

            if (varTipo!='$')
            {
                if (varTipo=='#')  // verifica se eh numero inteiro ou real
                {
                    uInput.fVal = atof((char*)answer);
                }
                else
                {
                    uInput.iVal = atoi((char*)answer);
                }

                memcpy(answer, uInput.vBytes, 4);
            }

            vTempPointer = (unsigned char*)pointerRunProg;
            if (*vTempPointer == 0x28)
            {
                vRetFV = findVariable(varName);
                if (vErroProc) return 0;

                if (!vRetFV)
                {
                    vErroProc = 4;
                    return 0;
                }

                vArray = 1;
            }

            if (!vArray)
            {
                // assign the value
                vRetFV = findVariable(varName);

                // Se nao existe variavel e inicio sentenca, cria variavel e atribui o valor
                if (!vRetFV)
                    createVariable(varName, answer, varTipo);
                else // se ja existe, altera
                    updateVariable((unsigned long*)(vRetFV + 3), answer, varTipo, 1);
            }
            else
            {
                updateVariable((unsigned long*)vRetFV, answer, varTipo, 2);
            }

            vTemTexto=2;
            nextToken();
            if (vErroProc) return 0;
        }

        if (vTemTexto==1 && *token==';')
            /* do nothing */;
        else if (vTemTexto==1 && *token!=';')
        {
            vErroProc = 14;
            return 0;
        }
        else if (vTemTexto!=1 && *token==';')
        {
            vErroProc = 14;
            return 0;
        }
        else if (tok!=EOL && tok!=FINISHED && *token!=':')
        {
            vErroProc = 14;
            return 0;
        }
    } while (*token==';');

    return 0;
}

#ifdef __NOT_USING_ON_BASIC__
//--------------------------------------------------------------------------------------
// Low Resolution Screen Mode (64x48)
// Syntaxe:
//          GR
//--------------------------------------------------------------------------------------
int basGr(void)
{
    vdp_init(TFT_MODE_MULTICOLOR, 0, 0, 0);
    return 0;
}

//--------------------------------------------------------------------------------------
// High Resolution Screen Mode (256x192)
// Syntaxe:
//          HGR
//--------------------------------------------------------------------------------------
int basHgr(void)
{
    vdp_init(TFT_MODE_G2, 0x0, 1, 0);
    vdp_set_bdcolor(TFT_BLACK);
    return 0;
}

//--------------------------------------------------------------------------------------
// Inverte as Cores de tela (COR FRENTE <> COR NORMAL)
// Syntaxe:
//          INVERSE
//
//    **********************************************************************************
//    ** SOMENTE PARA COMPATIBILIDADE, NO TMS91xx E TMS99xx NAO FUNCIONA COR POR CHAR **
//    **********************************************************************************
//--------------------------------------------------------------------------------------
int basInverse(void)
{
/*    unsigned char vTempCor;

    fgcolorAnt = fgcolor;
    bgcolorAnt = bgcolor;
    vTempCor = fgcolor;
    fgcolor = bgcolor;
    bgcolor = vTempCor;
    vdp_textcolor(fgcolor,bgcolor);*/

    return 0;
}

//--------------------------------------------------------------------------------------
// Volta as cores de tela as cores iniciais
// Syntaxe:
//          NORMAL
//
//    **********************************************************************************
//    ** SOMENTE PARA COMPATIBILIDADE, NO TMS91xx E TMS99xx NAO FUNCIONA COR POR CHAR **
//    **********************************************************************************
//--------------------------------------------------------------------------------------
int basNormal(void)
{
/*    fgcolor = fgcolorAnt;
    bgcolor = bgcolorAnt;
    vdp_textcolor(fgcolor,bgcolor);*/

    return 0;
}

//--------------------------------------------------------------------------------------
// Muda a cor do plot em baixa/alta resolucao (GR or HGR from basHcolor)
// Syntaxe:
//          COLOR=<color>
//--------------------------------------------------------------------------------------
int basColor(void)
{
    int ix = 0, iy = 0, iz = 0, iw = 0, vToken;
    unsigned char answer[20];
    int  *iVal = answer;
    unsigned char vTab, vColumn;
    unsigned char sqtdtam[10];
    unsigned char *vTempPointer;

    if (vdp_mode != TFT_MODE_MULTICOLOR && vdp_mode != TFT_MODE_G2)
    {
        vErroProc = 24;
        return 0;
    }

    vTempPointer = pointerRunProg;
    if (*vTempPointer != '=')
    {
        vErroProc = 3;
        return 0;
    }

    pointerRunProg = pointerRunProg + 1;
    vTempPointer = pointerRunProg;

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(&answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        if (value_type == '#')
        {
            *iVal = fppInt(*iVal);
            value_type = '%';
        }
    }

    fgcolor=(char)*iVal;

    value_type='%';

    return 0;
}

//--------------------------------------------------------------------------------------
// Coloca um dot ou preenche uma area com a color previamente definida
// Syntaxe:
//          PLOT <x entre 0 e 63>, <y entre 0 e 47>
//--------------------------------------------------------------------------------------
int basPlot(void)
{
    int ix = 0, iy = 0, iz = 0, iw = 0, vToken;
    unsigned char answer[20];
    int  *iVal = answer;
    unsigned char vx, vy;
    unsigned char sqtdtam[10];

    if (vdp_mode != TFT_MODE_MULTICOLOR)
    {
        vErroProc = 24;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(&answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        if (value_type == '#')
        {
            *iVal = fppInt(*iVal);
            value_type = '%';
        }
    }

    vx=(char)*iVal;

    if (token != ',')
    {
        vErroProc = 18;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        //putback();

        getExp(&answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        if (value_type == '#')
        {
            *iVal = fppInt(*iVal);
            value_type = '%';
        }
    }

    vy=(char)*iVal;

    vdp_plot_color(vx, vy, fgcolor);

    value_type='%';

    return 0;
}

//--------------------------------------------------------------------------------------
// Desenha uma linha horizontal de x1, y1 até x2, y1
// Syntaxe:
//          HLIN <x1>, <x2> at <y1>
//               x1 e x2 : de 0 a 63
//                    y1 : de 0 a 47
//
// Desenha uma linha vertical de x1, y1 até x1, y2
// Syntaxe:
//          VLIN <y1>, <y2> at <x1>
//                    x1 : de 0 a 63
//               y1 e y2 : de 0 a 47
//--------------------------------------------------------------------------------------
int basHVlin(unsigned char vTipo)   // 1 - HLIN, 2 - VLIN
{
    int ix = 0, iy = 0, iz = 0, iw = 0, vToken;
    unsigned char answer[20];
    int  *iVal = answer;
    unsigned char vx1, vx2, vy;
    unsigned char sqtdtam[10];

    if (vdp_mode != TFT_MODE_MULTICOLOR)
    {
        vErroProc = 24;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(&answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        if (value_type == '#')
        {
            *iVal = fppInt(*iVal);
            value_type = '%';
        }
    }

    vx1=(char)*iVal;

    if (token != ',')
    {
        vErroProc = 18;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        //putback();

        getExp(&answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        if (value_type == '#')
        {
            *iVal = fppInt(*iVal);
            value_type = '%';
        }
    }

    vx2=(char)*iVal;

    if (token != 0xBA) // AT Token
    {
        vErroProc = 18;
        return 0;
    }

    pointerRunProg = pointerRunProg + 1;

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(&answer);
        if (vErroProc) return 0;

        if (value_type == '$')
        {
            vErroProc = 16;
            return 0;
        }

        if (value_type == '#')
        {
            *iVal = fppInt(*iVal);
            value_type = '%';
        }
    }

    vy=(char)*iVal;

    if (vx2 < vx1)
    {
        ix = vx1;
        vx1 = vx2;
        vx2 = ix;
    }

    if (vTipo == 1)   // HLIN
    {
        for(ix = vx1; ix <= vx2; ix++)
            vdp_plot_color(ix, vy, fgcolor);
    }
    else   // VLIN
    {
        for(ix = vx1; ix <= vx2; ix++)
            vdp_plot_color(vy, ix, fgcolor);
    }
    value_type='%';

    return 0;
}

//--------------------------------------------------------------------------------------
//
// Syntaxe:
//
//--------------------------------------------------------------------------------------
int basScrn(void)
{
    int ix = 0, iy = 0, iz = 0, iw = 0, vToken;
    unsigned char answer[20];
    int *iVal = answer;
    int *tval = *token;
    unsigned char vx, vy;
    unsigned char sqtdtam[10];

    if (vdp_mode != TFT_MODE_MULTICOLOR)
    {
        vErroProc = 24;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    // Erro, primeiro caracter deve ser abre parenteses
    if (*tok == EOL || *tok == FINISHED || token_type != OPENPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(&answer);
        if (vErroProc) return 0;

        if (value_type != '%')
        {
            vErroProc = 16;
            return 0;
        }
    }

    vx=(char)*iVal;

    nextToken();
    if (vErroProc) return 0;

    if (token!=',')
    {
        vErroProc = 18;
        return 0;
    }

    nextToken();
    if (vErroProc) return 0;

    if (token_type == QUOTE) { /* is string, error */
        vErroProc = 16;
        return 0;
    }
    else { /* is expression */
        putback();

        getExp(&answer);
        if (vErroProc) return 0;

        if (value_type != '%')
        {
            vErroProc = 16;
            return 0;
        }
    }

    vy=(char)*iVal;

    nextToken();
    if (vErroProc) return 0;

    // Ultimo caracter deve ser fecha parenteses
    if (token_type!=CLOSEPARENT)
    {
        vErroProc = 15;
        return 0;
    }

    // Ler Aqui.. a cor e devolver em *tval
    *tval = vdp_read_color_pixel(vx,vy);

    value_type='%';

    return 0;
}

//--------------------------------------------------------------------------------------
//
// Syntaxe:
//
//--------------------------------------------------------------------------------------
int basHcolor(void)
{
    if (vdp_mode != TFT_MODE_G2)
    {
        vErroProc = 24;
        return 0;
    }

    basColor();
    if (vErroProc) return 0;

    return 0;
}

//--------------------------------------------------------------------------------------
//
// Syntaxe:
//
//--------------------------------------------------------------------------------------
int basHplot(void)
{
    int ix = 0, iy = 0, iz = 0, iw = 0, vToken;
    unsigned char answer[20];
    int  *iVal = answer;
    int rivx, rivy;
    unsigned long riy, rlvx, rlvy, vDiag;
    unsigned char vx, vy, vtemp;
    unsigned char sqtdtam[10];
    unsigned char vOper = 0;
    int x,y,addx,addy,dx,dy;
    long P;

    if (vdp_mode != TFT_MODE_G2)
    {
        vErroProc = 24;
        return 0;
    }

    do
    {
        nextToken();
        if (vErroProc) return 0;

        if (token != 0x86)
        {
            if (token_type == QUOTE) { // is string, error
                vErroProc = 16;
                return 0;
            }
            else { // is expression
                putback();

                getExp(&answer);
                if (vErroProc) return 0;

                if (value_type == '$')
                {
                    vErroProc = 16;
                    return 0;
                }

                if (value_type == '#')
                {
                    *iVal = fppInt(*iVal);
                    value_type = '%';
                }
            }

            vx = (unsigned char)*iVal;

            if (token != ',')
            {
                vErroProc = 18;
                return 0;
            }

            nextToken();
            if (vErroProc) return 0;

            if (token_type == QUOTE) { // is string, error
                vErroProc = 16;
                return 0;
            }
            else { // is expression
                //putback();

                getExp(&answer);
                if (vErroProc) return 0;

                if (value_type == '$')
                {
                    vErroProc = 16;
                    return 0;
                }

                if (value_type == '#')
                {
                    *iVal = fppInt(*iVal);
                    value_type = '%';
                }
            }

            vy = (unsigned char)*iVal;

            if (!vOper)
                vOper = 1;
        }
        else
        {
           // pointerRunProg = pointerRunProg + 1;
        }

        if (*tok == EOL || *tok == FINISHED || *token == 0x86)    // Fim de linha, programa ou *token
        {
            if (!vOper)
            {
                vOper = 2;
            }
            else if (vOper == 1)
            {
                *lastHgrX = vx;
                *lastHgrY = vy;

                if (token != 0x86)
                    vdp_plot_hires(vx, vy, fgcolor, bgcolor);
            }
            else if (vOper == 2)
            {
                if (vx == *lastHgrX && vy == *lastHgrY)
                    vdp_plot_hires(vx, vy, fgcolor, bgcolor);
                else
                {
                    dx = (vx - *lastHgrX);
                    dy = (vy - *lastHgrY);

                    if (dx < 0)
                        dx = dx * (-1);

                    if (dy < 0)
                        dy = dy * (-1);

                    x = *lastHgrX;
                    y = *lastHgrY;

                    if(*lastHgrX > vx)
                       addx = -1;
                    else
                       addx = 1;

                    if(*lastHgrY > vy)
                      addy = -1;
                    else
                      addy = 1;

                    if(dx >= dy)
                    {
                        P = (2 * dy) - dx;

                        for(ix = 1; ix <= (dx + 1); ix++)
                        {
                            vdp_plot_hires(x, y, fgcolor, 0);

                            if (P < 0)
                            {
                                P = P + (2 * dy);
                                x = (x + addx);
                            }
                            else
                            {
                                P = P + (2 * dy) - (2 * dx);
                                x = x + addx;
                                y = y + addy;
                            }
                        }
                    }
                    else
                    {
                        P = (2 * dx) - dy;

                        for(ix = 1; ix <= (dy +1); ix++)
                        {
                            vdp_plot_hires(x, y, fgcolor, 0);

                            if (P < 0)
                            {
                                P = P + (2 * dx);
                                y = y + addy;
                            }
                            else
                            {
                                P = P + (2 * dx) - (2 * dy);
                                x = x + addx;
                                y = y + addy;
                            }
                        }
                    }
                }

                *lastHgrX = vx;
                *lastHgrY = vy;
            }

            if (*token == 0x86)
            {
                pointerRunProg = pointerRunProg + 1;
            }
        }

        vOper = 2;
   } while (*token == 0x86); // TO Token

    value_type='%';

    return 0;
}
#endif
