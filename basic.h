#ifndef __BASIC_H
#define __BASIC_H

#ifdef __cplusplus
extern "C" {
#endif

#define __IMPORT_FILE_BAS__
//#define __COMMAND_LINE__

//#define __MAIN_PARAM__
#define __FUNC_TO_CALL__

// 36KB RAM Total
#define BASIC_SIZE_VAR       0x1000     // 4 KB
#define BASIC_SIZE_ARRAY_VAR 0x2000     // 8 KB
#define BASIC_SIZE_STRING    0x2000     // 8 KB
#define BASIC_SIZE_PROG      0x4000     // 16 KB

// Variable Default: Real Numbers
#define VARTYPEDEFAULT 0x23
const int FOR_NEST = 80;
const int SUB_NEST = 190;

#define FINISHED  0xE0
#define END       0xE1
#define EOL       0xE2

#define DELIMITER  1
#define VARIABLE  2
#define NUMBER    3
#define COMMAND   4
#define STRING    5
#define QUOTE     6
#define DOISPONTOS 7
#define OPENPARENT 8
#define CLOSEPARENT 9

#define TFT_MODE_TEXT 0
#define TFT_MODE_MULTICOLOR 1
#define TFT_MODE_G2 2

union uValue {
  int iVal;
  float fVal;
  unsigned char vBytes[4];
};

typedef struct  {
  unsigned char nameVar[3];  // variable name
  long endVar; // address off the counter variable
  union uValue target;  // target value
  union uValue step; // step inc/dec
  long progPosPointerRet;
} for_stack;

struct keyword_token
{
  char *keyword;
  int token;
};

typedef struct
{
  unsigned char tString[250];
  short tInt;
  long tLong;
  unsigned char tType;  // 0 - String, 1 - Int, 2 - Long
} typeInf;

unsigned char pProcess;
unsigned char pTypeLine; // 0x00 = Proxima linha tem "READY" e ">". 0x01 = Proxima linha tem somente ">"
unsigned long nextAddrLine; // Endereco da proxima linha disponivel pra ser incluida do basic
unsigned short firstLineNumber; // Numero de Linha mais Baixo
unsigned long addrFirstLineNumber; // Endereco do numero de linha mais baixo
unsigned long addrLastLineNumber; // Endereco do numero de linha mais baixo
unsigned long nextAddr; // usado para controleno runProg
unsigned long nextAddrSimpVar; // Endereco da proxima linha disponivel pra definir variavel
unsigned long nextAddrArrayVar; // Endereco da proxima linha disponivel pra definir array
unsigned long nextAddrString; // Endereco da proxima linha disponivel pra incluir string

unsigned char comandLineTokenized[256]; // Linha digitada sem numeros inicial e sem comandos basicos irá interpretada com tokens (255 Bytes)
unsigned char vParenteses; // Controle de Parenteses na linha inteira durante processamento
unsigned char vInicioSentenca; // Indica inicio de sentenca, sempre inicio analise ou depois de uma ",", ":", "THEN" ou "ELSE"
unsigned char doisPontos; // Se teve 2 pontos na linha e inicia novo comando como se fosse linha nova
unsigned short vErroProc; // Erro global
int ftos; // index to top of FOR stack
int gtos; // index to top of GOSUB stack

unsigned int randSeed; // Numero para ser usado no randomico
unsigned int antRandSeed;
unsigned char lastHgrX; //
unsigned char lastHgrY; //
unsigned long pointerRunProg; // Ponteiro da execucao do programa ou linha digitada
unsigned char tok;
unsigned char token_type;
unsigned char value_type;
unsigned long onErrGoto;
unsigned long changedPointer; // Se ouve mudanca de endereço (goto/gosub/for-next e etc), nao usa sequencia, usa o endereço definido
//unsigned char* token; //
unsigned char varName[256]; // ja esta ok
unsigned char traceOn; // Mostra numero linhas durante execucao, para debug
unsigned long gosubStack[SUB_NEST]; // ja esta ok stack for gosub/return
unsigned long vDataFirst; //
unsigned long vDataLineAtu; //
for_stack forStack[FOR_NEST]; // stack for FOR/NEXT loop
unsigned long atuVarAddr; // Endereco da variavel atualmente usada pelo basLet
unsigned char vdp_mode;
unsigned char vBufReceived;
char dataShow[80];
unsigned int fgcolor;
unsigned int bgcolor;
unsigned char debugON;
unsigned char debugON2;
unsigned char debugON3;
unsigned char debugON4;

// -------------------------------------------------------------------------------
// Mensagens de Erro
// -------------------------------------------------------------------------------
static char* listError[]= {
    /* 00 */ (char*)"?reserved 0",
    /* 01 */ (char*)"?Stopped",
    /* 02 */ (char*)"?No expression present",
    /* 03 */ (char*)"?Equals sign expected",
    /* 04 */ (char*)"?Not a variable",
    /* 05 */ (char*)"?Out of range",
    /* 06 */ (char*)"?Illegal quantity",
    /* 07 */ (char*)"?Line not found",
    /* 08 */ (char*)"?THEN expected",
    /* 09 */ (char*)"?TO expected",
    /* 10 */ (char*)"?Too many nested FOR loops",
    /* 11 */ (char*)"?NEXT without FOR",
    /* 12 */ (char*)"?Too many nested GOSUBs",
    /* 13 */ (char*)"?RETURN without GOSUB",
    /* 14 */ (char*)"?Syntax error",
    /* 15 */ (char*)"?Unbalanced parentheses",
    /* 16 */ (char*)"?Incompatible types",
    /* 17 */ (char*)"?Line number expected",
    /* 18 */ (char*)"?Comma Espected",
    /* 19 */ (char*)"?Timeout",
    /* 20 */ (char*)"?Load with Errors",
    /* 21 */ (char*)"?Size error",
    /* 22 */ (char*)"?Out of memory",
    /* 23 */ (char*)"?Variable name already exist",
    /* 24 */ (char*)"?Wrong mode resolution",
    /* 25 */ (char*)"?Illegal position",
    /* 26 */ (char*)"?Out of data"
};

// -------------------------------------------------------------------------------
// Tokens
//    0x80 - 0xBF : Commands
//    0xC0 - 0xEF : Functions
//    0xF0 - 0xFF : Operands
// -------------------------------------------------------------------------------
const int keywords_count = 67;

static const struct keyword_token keywords[] =
{                             // 1a 2a 3a 4a - versoes (-- : desenv/testar, ok : funcionando, .. : nao feito)
  {(char*)"LET", 		 0x80},   // ok ok ok ok
  {(char*)"PRINT", 	 0x81},   // ok ok ok ok
  {(char*)"IF", 		 0x82},   // .. ok ok ok
  {(char*)"THEN", 	 0x83},   // .. ok ok ok
  {(char*)"FOR", 		 0x85},   // .. .. ok
  {(char*)"TO", 		 0x86},   // .. .. ok
  {(char*)"NEXT", 	 0x87},   // .. .. ok
  {(char*)"STEP", 	 0x88},   // .. .. ok
  {(char*)"GOTO" , 	 0x89},   // .. .. ok ok
  {(char*)"GOSUB", 	 0x8A},   // .. .. ok ok
  {(char*)"RETURN",  0x8B},   // .. .. ok ok
  {(char*)"REM", 		 0x8C},   // .. .. ok ok
  {(char*)"INVERSE", 0x8D},   // .. .. ok
  {(char*)"NORMAL",  0x8E},   // .. .. ok
  {(char*)"DIM", 		 0x8F},   // .. .. ok
  {(char*)"ON",      0x91},   // .. .. ok
  {(char*)"INPUT", 	 0x92},   // .. ok ok
  {(char*)"GET",     0x93},   // .. ok ok
  {(char*)"VTAB",    0x94},   // .. .. ok ok
  {(char*)"HTAB",    0x95},   // .. .. ok ok
  {(char*)"HOME", 	 0x96},   // ok ok ok ok
  {(char*)"CLEAR", 	 0x97},   // .. .. ok ok
  {(char*)"DATA", 	 0x98},   // .. .. ok
  {(char*)"READ", 	 0x99},   // .. .. ok
  {(char*)"RESTORE", 0x9A},   // .. .. ok
  {(char*)"END",     0x9E},   // .. .. ok ok
  {(char*)"STOP",    0x9F},   // .. .. ok ok
  {(char*)"TEXT",    0xB0},   // .. .. ok
  {(char*)"GR",      0xB1},   // .. .. ok
  {(char*)"HGR",     0xB2},   // .. .. ok
  {(char*)"COLOR",   0xB3},   // .. .. ok
  {(char*)"PLOT",    0xB4},   // .. .. ok
  {(char*)"HLIN",    0xB5},   // .. .. ok
  {(char*)"VLIN",    0xB6},   // .. .. ok
  {(char*)"HCOLOR",  0xB8},   // .. .. ok
  {(char*)"HPLOT",   0xB9},   // .. .. ok
  {(char*)"AT",      0xBA},   // .. .. ok
  {(char*)"ONERR",   0xBB},   // .. .. ok
  {(char*)"ASC",     0xC4},   // .. .. ok ok
  {(char*)"PEEK",    0xCD},   // .. .. ok
  {(char*)"POKE",    0xCE},   // .. .. ok
  {(char*)"RND",     0xD1},   // .. .. ok ok
  {(char*)"LEN",     0xDB},   // ok ok ok ok
  {(char*)"VAL",     0xDC},   // ok ok ok ok
  {(char*)"STR$",    0xDD},   // ok ok ok ok
  {(char*)"SCRN",    0xE0},   // .. .. ok
  {(char*)"CHR$",    0xE1},   // ok ok ok ok
  {(char*)"FRE",     0xE2},   // ok ok ok ok
  {(char*)"SQRT",    0xE3},   // ok ok ok
  {(char*)"SIN",     0xE4},   // ok ok ok
  {(char*)"COS",     0xE5},   // ok ok ok
  {(char*)"TAN",     0xE6},   // ok ok ok
  {(char*)"LOG",     0xE7},   // .. .. ok
  {(char*)"EXP",     0xE8},   // .. .. ok
  {(char*)"SPC",     0xE9},   // .. .. ok ok
  {(char*)"TAB",     0xEA},   // .. .. ok ok
  {(char*)"MID$",    0xEB},   // .. .. ok ok
  {(char*)"RIGHT$",  0xEC},   // .. .. ok ok
  {(char*)"LEFT$",   0xED},   // .. .. ok ok
  {(char*)"INT",     0xEE},   // .. .. ok ok
  {(char*)"ABS",     0xEF},   // .. .. ok ok
  {(char*)"AND",     0xF3},   // ok ok ok
  {(char*)"OR",      0xF4},   // ok ok ok
  {(char*)">=",      0xF5},   // ok ok ok
  {(char*)"<=",      0xF6},   // ok ok ok
  {(char*)"<>",      0xF7},   // ok ok ok
  {(char*)"NOT",     0xF8}    // .. .. ok
};

const char operandsWithTokens[] = "+-*/^>=<";

/*static const struct keyword_token keywordsUnique[] =
{
  {"+",       0xFF},  // ok ok
  {"-",       0xFE},  // ok ok
  {"*",       0xFD},  // ok ok
  {"/",       0xFC},  // ok ok
  {"^",       0xFB},  // ok ok
  {">",       0xFA},  // ok ok
  {"=",       0xF9},  // ok ok
  {"<",       0xF8}   // ok ok
  {"§",       0xF8}   // ok ok - sem uso, somente ateh tirar isso
};*/

// -------------------------------------------------------------------------------
// Funcoes do Interpretador
// -------------------------------------------------------------------------------
void processLine(void);
void tokenizeLine(unsigned char *pTokenized);
int saveLine(char *pNumber, unsigned char *pLinha);
void listProg(unsigned char *pArg, unsigned short pPause);
void delLine(unsigned char *pArg);
void editLine(unsigned char *pNumber);
void runProg(unsigned char *pNumber);
void showErrorMessage(unsigned int pError, unsigned int pNumLine);
int executeToken(unsigned char pToken);
int findToken(unsigned char pToken);
unsigned long findNumberLine(unsigned short pNumber, unsigned char pTipoRet, unsigned char pTipoFind);
char createVariable(unsigned char* pVariable, unsigned char* pValor, char pType);
char updateVariable(unsigned long* pVariable, unsigned char* pValor, char pType, char pOper);
char createVariableArray(unsigned char* pVariable, char pType, unsigned int pNumDim, unsigned int *pDim);
long findVariable(unsigned char* pVariable);
void find_var(char *s, unsigned char *vTemp);
void putback(void);
int nextToken(void);
int ustrcmp(char *X, char *Y);
int isalphas(unsigned char c);
int isdigitus(unsigned char c);
int iswhite(unsigned char c);
int isdelim(unsigned char c);
void getExp(unsigned char *result);
void level2(unsigned char *result);
void level3(unsigned char *result);
void level30(unsigned char *result);
void level31(unsigned char *result);
void level32(unsigned char *result);
void level4(unsigned char *result);
void level5(unsigned char *result);
void level6(unsigned char *result);
void primitive(unsigned char *result);
void atithChar(unsigned char *r, unsigned char *h);
void arithInt(char o, char *r, char *h);
void arithReal(char o, char *r, char *h);
char logicalNumericFloat(unsigned char o, char *r, char *h, char vTipoRetVar);
void logicalNumericInt(unsigned char o, char *r, char *h);
void logicalString(unsigned char o, char *r, char *h);
void unaryInt(char o, int *r);
void unaryReal(char o, float *r);
int forFind(for_stack *i, unsigned char* endLastVar);
void forPush(for_stack i);
for_stack forPop(void);
void gosubPush(unsigned long i);
unsigned long gosubPop(void);
int procParam(unsigned char tipoRetorno, unsigned char temParenteses, unsigned char tipoSeparador, unsigned char qtdParam, unsigned char *tipoParams,  unsigned char *retParams);
unsigned char readChar(void);

// -------------------------------------------------------------------------------
// Funcoes dos Comandos Basic
// -------------------------------------------------------------------------------
int basLet(void);
int basPrint(void);
int basChr(void);
int basFre(void);
int basVal(void);
int basLen(void);
int basStr(void);
int basAsc(void);
int basLeftRightMid(char pTipo);
int basIf(void);
int basLet(void);
int basInputGet(unsigned char pSize);
int basFor(void);
int basNext(void);
int basOnErr(void);
int basGoto(void);
int basGosub(void);
int basReturn(void);
int basRnd(void);
int basVtab(void);
int basHtab(void);
int basEnd(void);
int basStop(void);
int basSpc(void);
int basTab(void);
int basXBasLoad(void);
int basInt(void);
int basAbs(void);
int basPeekPoke(char pTipo);
int basDim(void);
int basInverse(void);
int basNormal(void);
int basOnVar(void);
int basText(void);
int basGr(void);
int basHgr(void);
int basColor(void);
int basPlot(void);
int basHVlin(unsigned char vTipo);   // 1 - HLIN, 2 - VLIN
int basScrn(void);
int basHcolor(void);
int basHplot(void);
int basRead(void);
int basRestore(void);
int basTrig(unsigned char pFunc);

// -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------
// Funcoes Aritimeticas que Suportam Inteiros e Ponto Flutuante (Numeros Reais)
// -------------------------------------------------------------------------------
unsigned int powNum(unsigned int pbase, unsigned char pexp);
unsigned long floatStringToFpp(unsigned char* pFloat);
int fppTofloatString(unsigned long pFpp, unsigned char *buf);
void TRACE_ON(void);
void TRACE_OFF(void);
void floatToChar(float x, char *p);

#ifdef __cplusplus
}
#endif

#endif /* __BASIC_H */
