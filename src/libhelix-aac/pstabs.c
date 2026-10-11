/* ***** BEGIN LICENSE BLOCK *****
    Source last modified: $Id: pstabs.c,v 1.0 2023/10/01 00:00:00 $

    Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.

    The contents of this file, and the files included with this file,
    are subject to the current version of the RealNetworks Public
    Source License (the "RPSL") available at
    http://www.helixcommunity.org/content/rpsl unless you have licensed
    the file under the current version of the RealNetworks Community
    Source License (the "RPSL") available at
    http://www.helixcommunity.org/content/rcsl, in which case the RCSL
    will apply. You may also obtain the license terms directly from
    RealNetworks.  You may not use this file except in compliance with
    the RPSL or, if you have a valid RCSL with RealNetworks applicable
    to this file, the RCSL.  Please see the applicable RPSL or RCSL for
    the rights, obligations and limitations governing use of the
    contents of the file.

    This file is part of the Helix DNA Technology. RealNetworks is the
    developer of the Original Code and owns the copyrights in the
    portions it created.

    This file, and the files included with this file, is distributed
    and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
    ENJOYMENT OR NON-INFRINGEMENT.

    Technology Compatibility Kit Test Suite(s) Location:
      http://www.helixcommunity.org/content/tck

    Contributor(s):

 * ***** END LICENSE BLOCK ***** */

#include "ps.h"

#if defined(AAC_ENABLE_PS) && AAC_ENABLE_PS

/* Dequantization scale factor tables in PROGMEM (Q30 format, capped to 32-bit max) */

/* IID (Inter-channel Intensity Difference) scale factors c1 in Q30 */
const int iid_scale_tab[15] PROGMEM = {
    0x055db01a, /* -7 */
    0x0a04ad8a, /* -6 */
    0x0f977a65, /* -5 */
    0x17ac04a3, /* -4 */
    0x1f5eb7ff, /* -3 */
    0x26658333, /* -2 */
    0x2a95110e, /* -1 */
    0x2d413ccd, /*  0 */
    0x2fc732ab, /*  1 */
    0x3333dd97, /*  2 */
    0x37c8dc9d, /*  3 */
    0x3b7613c8, /*  4 */
    0x3e125c4d, /*  5 */
    0x3f360602, /*  6 */
    0x3fc64fab  /*  7 */
};

/* ICC cos(alpha) in Q30 */
const int icc_cos_tab[8] PROGMEM = {
    0x40000000,
    0x3efbe321,
    0x3d674023,
    0x393e4b8b,
    0x34e9515b,
    0x2d413ccd,
    0x1d033669,
    0x00000000
};

/* ICC sin(alpha) in Q30 */
const int icc_sin_tab[8] PROGMEM = {
    0x00000000,
    0x0b5bdf1d,
    0x120b973c,
    0x1c9f25c6,
    0x24015d80,
    0x2d413ccd,
    0x390bd586,
    0x40000000
};

/* Allpass filter fractional delay / feedback coefficients in Q30 */
const int alpha_tab[8] PROGMEM = {
    0x23d70a3d, /* 0.35 */
    0x20000000, /* 0.30 */
    0x1c28f5c3, /* 0.26 */
    0x1851eb85, /* 0.22 */
    0x147ae148, /* 0.18 */
    0x10a3d70a, /* 0.14 */
    0x0cccccccc, /* 0.10 */
    0x08f5c28f  /* 0.06 */
};

/* Sub-QMF FIR filter coefficients p_8[13] in Q30 fixed-point */
const int p8_13_20[7] PROGMEM = {
    0x007a82bc, /* 0.0074608295 */
    0x0174151f, /* 0.0227042095 */
    0x02e90c8a, /* 0.0454686593 */
    0x048259ca, /* 0.0704481023 */
    0x05f03d29, /* 0.0927847422 */
    0x06e6a17b, /* 0.1078233737 */
    0x0739c36a  /* 0.1129202396 */
};

/* Sub-QMF FIR filter coefficients p_4[13] in Q30 fixed-point */
const int p4_13_20[7] PROGMEM = {
    0x0002f232, /* 0.0001801856 */
    0x002e23f0, /* 0.0028162232 */
    0x0188686e, /* 0.0240108920 */
    0x052d9a30, /* 0.0809149448 */
    0x0b6863d0, /* 0.1782631558 */
    0x1224f8d0, /* 0.2835081283 */
    0x153ebce0  /* 0.3319525281 */
};

/* Sub-QMF FIR filter coefficients p_2[13] in Q30 fixed-point */
const int p2_13_20[7] PROGMEM = {
    0x032ad800, /* 0.0494802100 */
    0x0155b000, /* 0.0208552100 */
    0xfe97f400, /* -0.0219762100 */
    0xfba7f000, /* -0.0678842100 */
    0xfaf44800, /* -0.0789122100 */
    0x02a7b800, /* 0.0414842100 */
    0x11626000  /* 0.2716302100 */
};

#endif /* defined(AAC_ENABLE_PS) && AAC_ENABLE_PS */
