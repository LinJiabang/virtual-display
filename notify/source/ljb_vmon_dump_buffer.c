#define IS_PRINTABLE(c) ((31 < c) && (c < 127))

#include <stdio.h>
#include "ljb_vmon.h"

#define  LINE_SIZE       512

/*
 * Name:  LJB_VMON_DumpBuffer
 *
 * Definition:
 * 	VOID
 * 	LJB_VMON_DumpBuffer(
 *        __in UCHAR CONST *  pBuf,
 *        __in ULONG          BufSize
 * 		);
 *
 * Description:
 *
 * Return Value:
 *    None
 *
 */

VOID
LJB_VMON_DumpBuffer(
	__in UCHAR  *      pBuf,
    __in ULONG         BufSize
    )
    {
    CHAR    LineBuf[LINE_SIZE];
    UCHAR * pLine;
    UINT    LineOffset;
    UINT    BytesRemained;

    pLine = pBuf;
    BytesRemained = BufSize;
    LineOffset = 0;;
    OutputDebugString("\t00 01 02 03 04 05 06 07-08 09 0A 0B 0C 0D 0E 0F\n");
    OutputDebugString("\t===============================================\n");
    while (BytesRemained > 0)
        {
        if (BytesRemained >= 16)
            {
            ZeroMemory(LineBuf, LINE_SIZE);
            (VOID) _snprintf_s(LineBuf, LINE_SIZE, _TRUNCATE,
                "%04x\t"
                "%02X %02X %02X %02X %02X %02X %02X %02X-"
                "%02X %02X %02X %02X %02X %02X %02X %02X "
                "\t"
                "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                LineOffset,
                pLine[0], pLine[1], pLine[2], pLine[3],
                pLine[4], pLine[5], pLine[6], pLine[7],
                pLine[8], pLine[9], pLine[10], pLine[11],
                pLine[12], pLine[13], pLine[14], pLine[15],
                IS_PRINTABLE(pLine[0]) ? pLine[0] : '.',
                IS_PRINTABLE(pLine[1]) ? pLine[1] : '.',
                IS_PRINTABLE(pLine[2]) ? pLine[2] : '.',
                IS_PRINTABLE(pLine[3]) ? pLine[3] : '.',
                IS_PRINTABLE(pLine[4]) ? pLine[4] : '.',
                IS_PRINTABLE(pLine[5]) ? pLine[5] : '.',
                IS_PRINTABLE(pLine[6]) ? pLine[6] : '.',
                IS_PRINTABLE(pLine[7]) ? pLine[7] : '.',
                IS_PRINTABLE(pLine[8]) ? pLine[8] : '.',
                IS_PRINTABLE(pLine[9]) ? pLine[9] : '.',
                IS_PRINTABLE(pLine[10]) ? pLine[10] : '.',
                IS_PRINTABLE(pLine[11]) ? pLine[11] : '.',
                IS_PRINTABLE(pLine[12]) ? pLine[12] : '.',
                IS_PRINTABLE(pLine[13]) ? pLine[13] : '.',
                IS_PRINTABLE(pLine[14]) ? pLine[14] : '.',
                IS_PRINTABLE(pLine[15]) ? pLine[15] : '.'
                );
            OutputDebugString(LineBuf);
            LineOffset += 16;
            pLine += 16;
            BytesRemained -= 16;
            }
        else
            {
            CHAR    TmpBuf[LINE_SIZE];
            UINT    i;
            UINT    PaddingSize;

            ZeroMemory(LineBuf, LINE_SIZE);
            ZeroMemory(TmpBuf, LINE_SIZE);
            PaddingSize = 16 - BytesRemained;
            _snprintf_s(LineBuf, LINE_SIZE,  _TRUNCATE,
                "%x\t",
                LineOffset
                );
             for (i = 0; i < BytesRemained; i++)
                {
                /*
                 print the main part
                 */
                if (i == 7)
                    {
                    (VOID) _snprintf_s(TmpBuf, LINE_SIZE, _TRUNCATE,
                        "%02X-",
                        pLine[i]
                        );
                    (VOID) strcat_s(LineBuf, LINE_SIZE, TmpBuf);
                    }
                else
                    {
                    (VOID) _snprintf_s(TmpBuf, LINE_SIZE, _TRUNCATE,
                        "%02X ",
                        pLine[i]
                        );
                    (VOID) strcat_s(LineBuf, LINE_SIZE, TmpBuf);
                    }
                }

            /*
             Now the padding space
             */
            for (i = 0; i < PaddingSize; i++)
                {
                (VOID) strcat_s(LineBuf, LINE_SIZE, "   ");
                }

            /*
             The last, the printable chars
             */
             strcat_s((CHAR*) LineBuf, LINE_SIZE, "\t");
             for (i = 0; i < BytesRemained; i++)
                {
                (VOID) _snprintf_s(TmpBuf, LINE_SIZE, _TRUNCATE,
                        "%c",
                        IS_PRINTABLE(pLine[i]) ? pLine[i] : '.'
                        );
                (VOID) strcat_s(LineBuf, LINE_SIZE, TmpBuf);
                }
            strcat_s(LineBuf, LINE_SIZE, "\n");
            OutputDebugString(LineBuf);
            LineOffset += BytesRemained;
            pLine += BytesRemained;
            BytesRemained = 0;
            }
        }
    }
