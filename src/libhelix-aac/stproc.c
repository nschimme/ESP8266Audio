/* ***** BEGIN LICENSE BLOCK *****
    Source last modified: $Id: stproc.c,v 1.3 2005/05/24 16:01:55 albertofloyd Exp $

    Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.

    The contents of this file, and the files included with this file,
    are subject to the current version of the RealNetworks Public
    Source License (the "RPSL") available at
    http://www.helixcommunity.org/content/rpsl unless you have licensed
    the file under the current version of the RealNetworks Community
    Source License (the "RCSL") available at
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

/**************************************************************************************
    Fixed-point HE-AAC decoder
    Jon Recker (jrecker@real.com)
    February 2005

    stproc.c - mid-side and intensity stereo processing
 **************************************************************************************/

#include "coder.h"
#include "assembly.h"

/*  pow14[0][i] = -pow(2, i/4.0)
    pow14[1][i] = +pow(2, i/4.0)

    i = [0,1,2,3]
    format = Q30
*/


/**************************************************************************************
    Function:    StereoProcessGroup

    Description: apply mid-side and intensity stereo to group of transform coefficients

    Inputs:      dequantized transform coefficients for both channels
                pointer to appropriate scalefactor band table
                mid-side mask enabled flag
                buffer with mid-side mask (one bit for each scalefactor band)
                bit offset into mid-side mask buffer
                max coded scalefactor band
                buffer of codebook indices for right channel
                buffer of scalefactors for right channel, range = [0, 256]

    Outputs:     updated transform coefficients in Q(FBITS_OUT_DQ_OFF)
                updated minimum guard bit count for both channels

    Return:      none

    Notes:       assume no guard bits in input
                gains 0 int bits
 **************************************************************************************/
static void StereoProcessGroup(int *coefL, int *coefR, const /*short*/ int *sfbTab,
                               int msMaskPres, unsigned char *msMaskPtr, int msMaskOffset, int maxSFB,
                               unsigned char *cbRight, short *sfRight, int *gbCurrent) {
    //fb
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
    static const int pow14[2][4] PROGMEM = {
        { 0xc0000000, 0xb3e407d7, 0xa57d8666, 0x945d819b },
        { 0x40000000, 0x4c1bf829, 0x5a82799a, 0x6ba27e65 }
    };
#pragma GCC diagnostic pop

    int sfb, width, cbIdx, sf, cl, cr, scalef, scalei;
    int gbMaskL, gbMaskR;
    unsigned char msMask;

    msMask = (*msMaskPtr++) >> msMaskOffset;
    gbMaskL = 0;
    gbMaskR = 0;

    for (sfb = 0; sfb < maxSFB; sfb++) {
        width = sfbTab[sfb + 1] - sfbTab[sfb];	/* assume >= 0 (see sfBandTabLong/sfBandTabShort) */
        cbIdx = cbRight[sfb];

        if (cbIdx == 14 || cbIdx == 15) {
            /* intensity stereo */
            if (msMaskPres == 1 && (msMask & 0x01)) {
                cbIdx ^= 0x01;    /* invert_intensity(): 14 becomes 15, or 15 becomes 14 */
            }
            sf = -sfRight[sfb];				/* negative since we use identity 0.5^(x) = 2^(-x) (see spec) */
            cbIdx &= 0x01;					/* choose - or + scale factor */
            scalef = pow14[cbIdx][sf & 0x03];
            scalei = (sf >> 2) + 2;			/* +2 to compensate for scalef = Q30 */

            if (scalei > 0) {
                if (scalei > 30) {
                    scalei = 30;
                }
                do {
                    cr = MULSHIFT32(*coefL++, scalef);
                    CLIP_2N(cr, 31 - scalei);
                    cr <<= scalei;
                    gbMaskR |= FASTABS(cr);
                    *coefR++ = cr;
                } while (--width);
            } else {
                scalei = -scalei;
                if (scalei > 31) {
                    scalei = 31;
                }
                do {
                    cr = MULSHIFT32(*coefL++, scalef) >> scalei;
                    gbMaskR |= FASTABS(cr);
                    *coefR++ = cr;
                } while (--width);
            }
        } else if (cbIdx != 13 && ((msMaskPres == 1 && (msMask & 0x01)) || msMaskPres == 2)) {
            /* mid-side stereo (assumes no GB in inputs) */
            do {
                cl = *coefL;
                cr = *coefR;

                if ((FASTABS(cl) | FASTABS(cr)) >> 30) {
                    /* avoid overflow (rare) */
                    cl >>= 1;
                    sf = cl + (cr >> 1);	CLIP_2N(sf, 30); 	sf <<= 1;
                    cl = cl - (cr >> 1);	CLIP_2N(cl, 30); 	cl <<= 1;
                } else {
                    /* usual case */
                    sf = cl + cr;
                    cl -= cr;
                }

                *coefL++ = sf;
                gbMaskL |= FASTABS(sf);
                *coefR++ = cl;
                gbMaskR |= FASTABS(cl);
            } while (--width);

        } else {
            /* nothing to do */
            coefL += width;
            coefR += width;
        }

        /* get next mask bit (should be branchless on ARM) */
        msMask >>= 1;
        if (++msMaskOffset == 8) {
            msMask = *msMaskPtr++;
            msMaskOffset = 0;
        }
    }

    cl = CLZ(gbMaskL) - 1;
    if (gbCurrent[0] > cl) {
        gbCurrent[0] = cl;
    }

    cr = CLZ(gbMaskR) - 1;
    if (gbCurrent[1] > cr) {
        gbCurrent[1] = cr;
    }

    return;
}

/**************************************************************************************
    Function:    StereoProcess

    Description: apply mid-side and intensity stereo, if enabled

    Inputs:      valid AACDecInfo struct (including dequantized transform coefficients)

    Outputs:     updated transform coefficients in Q(FBITS_OUT_DQ_OFF)
                updated minimum guard bit count for both channels

    Return:      0 if successful, -1 if error
 **************************************************************************************/
int StereoProcess(AACDecInfo *aacDecInfo) {
    PSInfoBase *psi;
    ICSInfo *icsInfo;
    int gp, win, nSamps, msMaskOffset;
    int *coefL, *coefR;
    unsigned char *msMaskPtr;
    const /*short*/ int *sfbTab;

    /* validate pointers */
    if (!aacDecInfo || !aacDecInfo->psInfoBase) {
        return -1;
    }
    psi = (PSInfoBase *)(aacDecInfo->psInfoBase);

    /* mid-side and intensity stereo require common_window == 1 (see MPEG4 spec, Correction 2, 2004) */
    if (psi->commonWin != 1 || aacDecInfo->currBlockID != AAC_ID_CPE) {
        return 0;
    }

    /* nothing to do */
    if (!psi->msMaskPresent && !psi->intensityUsed[1]) {
        return 0;
    }

    icsInfo = &(psi->icsInfo[0]);
    if (icsInfo->winSequence == 2) {
        sfbTab = sfBandTabShort + sfBandTabShortOffset[psi->sampRateIdx];
        nSamps = NSAMPS_SHORT;
    } else {
        sfbTab = sfBandTabLong + sfBandTabLongOffset[psi->sampRateIdx];
        nSamps = NSAMPS_LONG;
    }
    coefL = psi->coef[0];
    coefR = psi->coef[1];

    /* do fused mid-side/intensity processing for each block (one long or eight short) */
    msMaskOffset = 0;
    msMaskPtr = psi->msMaskBits;
    for (gp = 0; gp < icsInfo->numWinGroup; gp++) {
        for (win = 0; win < icsInfo->winGroupLen[gp]; win++) {
            StereoProcessGroup(coefL, coefR, sfbTab, psi->msMaskPresent,
                               msMaskPtr, msMaskOffset, icsInfo->maxSFB, psi->sfbCodeBook[1] + gp * icsInfo->maxSFB,
                               psi->scaleFactors[1] + gp * icsInfo->maxSFB, psi->gbCurrent);
            coefL += nSamps;
            coefR += nSamps;
        }
        /* we use one bit per sfb, so there are maxSFB bits for each window group */
        msMaskPtr += (msMaskOffset + icsInfo->maxSFB) >> 3;
        msMaskOffset = (msMaskOffset + icsInfo->maxSFB) & 0x07;
    }

    ASSERT(coefL == psi->coef[0] + 1024);
    ASSERT(coefR == psi->coef[1] + 1024);

    return 0;
}

/**************************************************************************************
    Function:    AACDownmixPCM

    Description: fixed-point adaptation of FAAD3 downmix_pcm

    Inputs:      PCM buffer, number of channels, number of samples per channel, mode (1=mono, 2=stereo)

    Outputs:     downmixed PCM in pcm buffer

    Return:      number of output channels (1 for mono, 2 for stereo)
 **************************************************************************************/
int AACDownmixPCM(short *pcm, int num_chs, int num_samps_per_ch, int mode) {
    int i, c = -1, ls = -1, rs = -1;
    int gain_q30, k_q30, lsw_q30;
    int lo, ro;

    if (mode == 0 || num_chs < 2 || (mode == 2 && num_chs == 2)) {
        return num_chs;
    }

    switch (num_chs) {
    case 3: c = 2; break;
    case 4: c = 2; ls = 3; rs = 3; break;
    case 5: c = 2; ls = 3; rs = 4; break;
    case 6: c = 2; ls = 4; rs = 5; break; /* 5.1: L(0), R(1), C(2), LFE(3), LS(4), RS(5) */
    case 8: c = 2; ls = 4; rs = 5; break; /* 7.1 */
    default: break;
    }

    k_q30 = 0x5a82799a; /* 0.70710678 in Q30 */

    /* Precalculated Q30 gains to prevent overflow clipping */
    switch (num_chs) {
    case 3:  gain_q30 = 0x257a7802; break; /* 1 / (1 + k) = 0.585786 */
    case 4:  gain_q30 = 0x1f0f1880; break; /* 1 / (1 + 1.5*k) = 0.485281 */
    case 5:
    case 6:  gain_q30 = 0x1a82799a; break; /* 1 / (1 + 2*k) = 0.414214 */
    case 8:  gain_q30 = 0x1480f2d4; break; /* 1 / (1 + 3*k) = 0.320377 */
    default: gain_q30 = 0x40000000; break; /* 1.0 in Q30 */
    }

    lsw_q30 = (ls >= 0 && ls == rs) ? (k_q30 >> 1) : k_q30;

    if (mode == 1) { /* Mono downmix */
        for (i = 0; i < num_samps_per_ch; i++) {
            lo = pcm[i * num_chs + 0];
            ro = pcm[i * num_chs + 1];

            if (c >= 0) {
                int c_val = pcm[i * num_chs + c];
                lo += MULSHIFT32(c_val << 2, k_q30);
                ro += MULSHIFT32(c_val << 2, k_q30);
            }
            if (ls >= 0) {
                lo += MULSHIFT32(pcm[i * num_chs + ls] << 2, lsw_q30);
                ro += MULSHIFT32(pcm[i * num_chs + rs] << 2, lsw_q30);
            }
            if (num_chs == 8) {
                lo += MULSHIFT32(pcm[i * num_chs + 6] << 2, k_q30);
                ro += MULSHIFT32(pcm[i * num_chs + 7] << 2, k_q30);
            }

            if (num_chs > 2) {
                lo = MULSHIFT32(lo << 2, gain_q30);
                ro = MULSHIFT32(ro << 2, gain_q30);
            }

            pcm[i] = CLIPTOSHORT((lo + ro) >> 1);
        }
        return 1;
    } else { /* Stereo downmix */
        for (i = 0; i < num_samps_per_ch; i++) {
            lo = pcm[i * num_chs + 0];
            ro = pcm[i * num_chs + 1];

            if (c >= 0) {
                int c_val = pcm[i * num_chs + c];
                lo += MULSHIFT32(c_val << 2, k_q30);
                ro += MULSHIFT32(c_val << 2, k_q30);
            }
            if (ls >= 0) {
                lo += MULSHIFT32(pcm[i * num_chs + ls] << 2, lsw_q30);
                ro += MULSHIFT32(pcm[i * num_chs + rs] << 2, lsw_q30);
            }
            if (num_chs == 8) {
                lo += MULSHIFT32(pcm[i * num_chs + 6] << 2, k_q30);
                ro += MULSHIFT32(pcm[i * num_chs + 7] << 2, k_q30);
            }

            if (num_chs > 2) {
                lo = MULSHIFT32(lo << 2, gain_q30);
                ro = MULSHIFT32(ro << 2, gain_q30);
            }

            pcm[i * 2 + 0] = CLIPTOSHORT(lo);
            pcm[i * 2 + 1] = CLIPTOSHORT(ro);
        }
        return 2;
    }
}
