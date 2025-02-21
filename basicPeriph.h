#ifndef __BASIC_PERIPH_H
#define __BASIC_PERIPH_H

int out_width, out_height, out_heighttotal;
int TFT_FATOR_COL = 12;
int TFT_FATOR_ROW = 16;

#define STM32
//#define ARDUINO
//#define PIC16F
//#define PIC18F
//#define PIC24F
//#define PIC32F

#define OUT_MCUFRIEND_kbv
//#define OUT_VDP9118
//#define OUT_SERIAL

#define IN_TOUCH
//#define IN_PS2
//#define IN_SERIAL

#define KEY_NOWAIT 0
#define KEY_WAIT 1

#ifdef STM32
    #include "cmsis_os.h"
    #include "stm32f4xx_hal.h"
    #include "tm_stm32_gpio.h"
    #include "mmsj800.h"
    /*extern osMessageQId kbdDataQueueHandle;
    void ShowKeyboard(KbdData *pKbdDataPar, char pRtos);
    void HideKeyboard(KbdData *pKbdDataPar, char pRtos);*/
    extern void funcKey(BYTE vambiente, BYTE vshow, BYTE venter, BYTE vtipo, WORD x, WORD y);
    KbdData pKbdData;
#elif defined(ARDUINO)
    // TBD
#elif defined(PIC16F)
    // TBD
#elif defined(PIC18F)
    // TBD
#elif defined(PIC24F)
    // TBD
#elif defined(PIC32F)
    // TBD
#endif

#ifdef IN_TOUCH
    #include "TouchScreen.h"
    extern TouchScreen ts;
    extern void TCHVerif(TSData *tsPosition, char pRtos, KbdData *pKbdDataPar);
#elif defined(IN_PS2)
    // TBD
#elif defined(IN_SERIAL)
    // TBD
#endif

#ifdef OUT_MCUFRIEND_kbv
    #include "MCUFRIEND_kbv.h"
    #include "Fonts/FreeMono9pt7b.h"
    #include "Fonts/DialogInput_plain_12.h"
    extern MCUFRIEND_kbv tft;
#elif defined(OUT_VDP9118)
    // TBD
#elif defined(OUT_SERIAL)
    // TBD
#endif

//-----------------------------------------------------
// Saida Padrao. Escolher o Tipo de saida, ou customize
//-----------------------------------------------------
void outch (char c)
{
#ifdef OUT_MCUFRIEND_kbv
    tft.write(c);
#elif defined(OUT_VDP9118)
    // TBD
#elif defined(OUT_SERIAL)
    // TBD
#endif
}

void outs (char* s)
{
#ifdef OUT_MCUFRIEND_kbv
    tft.print(s);
#elif defined(OUT_VDP9118)
    // TBD
#elif defined(OUT_SERIAL)
    // TBD
#endif
}

void outsln (char* s)
{
#ifdef OUT_MCUFRIEND_kbv
    tft.println(s);
#elif defined(OUT_VDP9118)
    // TBD
#elif defined(OUT_SERIAL)
    // TBD
#endif
}

int16_t outGetWidth(void)
{
    return tft.width();
}

int16_t outGetHeight(void)
{
    return tft.height();
}

int16_t outGetPosX(void)
{
    return tft.getPosX();
}

int16_t outGetPosY(void)
{
    return tft.getPosY();
}

void outSetCursor(int c, int r)
{
    tft.setCursor(c, r);
}

void outFillScr(int color)
{
    tft.fillRect(0, 0, out_width, out_height, color);
}

void outFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    tft.fillRect(x, y, w, h, color);
}

void outDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    tft.drawRect(x, y, w, h, color);
}

void outSetTextColor(int fg, int bg)
{
    tft.setTextColor(fg, bg);
}

void outSetRotation(char r)
{
    tft.setRotation(r);
}

uint8_t outGetRotation(void)
{
    return tft.getRotation();
}

void outVertScroll(int16_t top, int16_t scrollines, int16_t offset)
{
    tft.vertScroll(top, scrollines, offset);
}

void outSetFont(const GFXfont *f)
{
    tft.setFont(f);
}
//-----------------------------------------------------
// Entrada Padrao. Escolher o Tipo de entrada, ou customize
//-----------------------------------------------------
char inch(char pTipo)
{
#ifdef IN_TOUCH
    char c = 0;
    TSData tsPos;

    if (pTipo)  // Para e Espera
    {
        while (1)
        {
            TCHVerif(&tsPos, 0, &pKbdData);

            if (tsPos.x != 0 || tsPos.y != 0)
            {
                if (pKbdData.pShow)
                {
                    ShowKeyboard(&pKbdData, 0);
                }
                else if (tsPos.keyBuffer != 0)
                {
                    c = tsPos.keyBuffer;
                    break;
                }
            }
        }
    }
    else // Retorna mesmo nao tendo lido nada
    {
        TCHVerif(&tsPos, 0, &pKbdData);

        if (tsPos.x != 0 || tsPos.y != 0)
        {
            if (pKbdData.pShow)
                ShowKeyboard(&pKbdData, 0);

            c = tsPos.keyBuffer;
        }
    }

    return c;
#elif defined(IN_PS2)
    // TBD
#elif defined(IN_SERIAL)
    // TBD
#endif
}
#endif
