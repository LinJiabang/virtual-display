/*
 * ljb_proxykmd_utility.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "ljb_proxykmd.h"
#include <emmintrin.h>

VOID
LJB_PROXYKMD_DelayMs(
    __in LONG  DelayInMs
    )
{
    LARGE_INTEGER   Timeout;

    /*
     * Timeout expressed in 100 ns unit. 1 ms = 10*1000 units
     */
    Timeout.QuadPart = -(DelayInMs * 10 * 1000);
    KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
}

SIZE_T
LJB_PROXYKMD_FastMemCpy(
    __out PVOID pDst,
    __in PVOID  pSrc,
    __in SIZE_T len
    )
{
    ULONG_PTR CONST     SrcAddr = (ULONG_PTR) pSrc;
    ULONG_PTR CONST     DstAddr = (ULONG_PTR) pDst;
    BOOLEAN CONST       bSrcAligned  = ((SrcAddr & 0x0F) == 0);
    BOOLEAN CONST       bDstAligned  = ((DstAddr & 0x0F) == 0);
    UCHAR *             pSrcPtr;
    UCHAR *             pDstPtr;
    BOOLEAN             bIsOverlapped;
    SIZE_T              BytesToCopy;
    __m128i             reg[8];

    bIsOverlapped = FALSE;
    if (((SrcAddr + 128) > DstAddr) || ((DstAddr + 128) > SrcAddr))
        bIsOverlapped = TRUE;

    pSrcPtr = pSrc;
    pDstPtr = pDst;
    BytesToCopy = len;
    while (BytesToCopy > 128)
    {
        if (bSrcAligned)
        {
            reg[0] =  _mm_load_si128((__m128i *) (pSrcPtr + 0x00));
            reg[1] =  _mm_load_si128((__m128i *) (pSrcPtr + 0x10));
            reg[2] =  _mm_load_si128((__m128i *) (pSrcPtr + 0x20));
            reg[3] =  _mm_load_si128((__m128i *) (pSrcPtr + 0x30));
            reg[4] =  _mm_load_si128((__m128i *) (pSrcPtr + 0x40));
            reg[5] =  _mm_load_si128((__m128i *) (pSrcPtr + 0x50));
            reg[6] =  _mm_load_si128((__m128i *) (pSrcPtr + 0x60));
            reg[7] =  _mm_load_si128((__m128i *) (pSrcPtr + 0x70));
        }
        else
        {
            reg[0] =  _mm_loadu_si128((__m128i *) (pSrcPtr + 0x00));
            reg[1] =  _mm_loadu_si128((__m128i *) (pSrcPtr + 0x10));
            reg[2] =  _mm_loadu_si128((__m128i *) (pSrcPtr + 0x20));
            reg[3] =  _mm_loadu_si128((__m128i *) (pSrcPtr + 0x30));
            reg[4] =  _mm_loadu_si128((__m128i *) (pSrcPtr + 0x40));
            reg[5] =  _mm_loadu_si128((__m128i *) (pSrcPtr + 0x50));
            reg[6] =  _mm_loadu_si128((__m128i *) (pSrcPtr + 0x60));
            reg[7] =  _mm_loadu_si128((__m128i *) (pSrcPtr + 0x70));
        }

        if (bDstAligned)
        {
            _mm_store_si128((__m128i *) (pDstPtr + 0x00), reg[0]);
            _mm_store_si128((__m128i *) (pDstPtr + 0x10), reg[1]);
            _mm_store_si128((__m128i *) (pDstPtr + 0x20), reg[2]);
            _mm_store_si128((__m128i *) (pDstPtr + 0x30), reg[3]);
            _mm_store_si128((__m128i *) (pDstPtr + 0x40), reg[4]);
            _mm_store_si128((__m128i *) (pDstPtr + 0x50), reg[5]);
            _mm_store_si128((__m128i *) (pDstPtr + 0x60), reg[6]);
            _mm_store_si128((__m128i *) (pDstPtr + 0x70), reg[7]);
        }
        else
        {
            _mm_storeu_si128((__m128i *) (pDstPtr + 0x00), reg[0]);
            _mm_storeu_si128((__m128i *) (pDstPtr + 0x10), reg[1]);
            _mm_storeu_si128((__m128i *) (pDstPtr + 0x20), reg[2]);
            _mm_storeu_si128((__m128i *) (pDstPtr + 0x30), reg[3]);
            _mm_storeu_si128((__m128i *) (pDstPtr + 0x40), reg[4]);
            _mm_storeu_si128((__m128i *) (pDstPtr + 0x50), reg[5]);
            _mm_storeu_si128((__m128i *) (pDstPtr + 0x60), reg[6]);
            _mm_storeu_si128((__m128i *) (pDstPtr + 0x70), reg[7]);
        }
        pSrcPtr += 0x80;
        pDstPtr += 0x80;
        BytesToCopy -= 0x80;
    }

    if (BytesToCopy > 0)
    {
        UCHAR   TmpBuf[128];

        if (bIsOverlapped)
        {
            RtlCopyMemory(TmpBuf, pSrcPtr, BytesToCopy);
            RtlCopyMemory(pDstPtr, TmpBuf, BytesToCopy);
        }
        else
        {
            RtlCopyMemory(pDstPtr, pSrcPtr, BytesToCopy);
        }
        pSrcPtr += BytesToCopy;
        pDstPtr += BytesToCopy;
        BytesToCopy = 0;
    }
    return len;
}
