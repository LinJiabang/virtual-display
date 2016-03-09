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
