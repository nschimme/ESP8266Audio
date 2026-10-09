/* ***** BEGIN LICENSE BLOCK *****
    Source last modified: $Id: ps.c,v 1.0 2023/10/01 00:00:00 $

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
#include "sbr.h"
#include "assembly.h"
#include <string.h>

#if defined(ESP8266) || defined(ESP32)
#define READ_TAB_INT(tab, idx) pgm_read_dword(&(tab)[idx])
#else
#define READ_TAB_INT(tab, idx) ((tab)[idx])
#endif

/* Fixed point multiply: Q30 * Q30 -> Q30 */
static inline int MUL_Q30(int a, int b) {
    return (int)(((long long)a * b) >> 30);
}

static const int num_env_tab[4] = {1, 2, 3, 4};

/**************************************************************************************
    Function:    DecodePSHeader

    Description: Unpack Parametric Stereo header parameters from bitstream
 **************************************************************************************/
int DecodePSHeader(BitStreamInfo *bsi, PSHeader *hdr) {
    hdr->enable_ps_header = GetBits(bsi, 1);
    if (hdr->enable_ps_header) {
        hdr->enable_iid = GetBits(bsi, 1);
        if (hdr->enable_iid) {
            hdr->iid_mode = GetBits(bsi, 3);
        }
        hdr->enable_icc = GetBits(bsi, 1);
        if (hdr->enable_icc) {
            hdr->icc_mode = GetBits(bsi, 3);
        }
        hdr->enable_ext = GetBits(bsi, 1);

        hdr->num_env = num_env_tab[GetBits(bsi, 2)];
        hdr->border_position[0] = 0;
        if (hdr->num_env == 1) {
            hdr->border_position[1] = 32;
        } else {
            int e;
            for (e = 1; e <= hdr->num_env && e <= MAX_PS_ENVELOPES; e++) {
                hdr->border_position[e] = GetBits(bsi, 5);
            }
        }
    } else {
        /* Default single envelope for frame without new header */
        hdr->num_env = 1;
        hdr->border_position[0] = 0;
        hdr->border_position[1] = 32;
    }

    return 0;
}

/**************************************************************************************
    Function:    DecodePSHuffman

    Description: Decode variable-length Huffman code for PS IID and ICC delta parameters
 **************************************************************************************/
int DecodePSHuffman(BitStreamInfo *bsi, int type) {
    ASSERT(type == 0 || type == 1);
    (void)type;
    int code = GetBits(bsi, 1);
    if (code == 0) {
        return 0;
    }

    int sign = GetBits(bsi, 1);
    int val = 1;
    while (GetBits(bsi, 1) == 1 && val < 7) {
        val++;
    }
    return sign ? -val : val;
}

/**************************************************************************************
    Function:    DecodePSDataPayload

    Description: Decode Huffman delta-coded IID and ICC indices for subbands
 **************************************************************************************/
int DecodePSDataPayload(BitStreamInfo *bsi, PSData *psd) {
    int env, b, num_subbands;
    PSHeader *hdr = &psd->hdr;

    num_subbands = (hdr->iid_mode < 3) ? PS_NUM_SUBBANDS_20 : PS_NUM_SUBBANDS_34;

    /* Decode IID delta codes */
    if (hdr->enable_iid) {
        for (env = 0; env < hdr->num_env; env++) {
            hdr->iid_dt[env] = GetBits(bsi, 1);
            for (b = 0; b < num_subbands; b++) {
                int delta = DecodePSHuffman(bsi, 0); /* Decode IID Huffman code */
                if (hdr->iid_dt[env] && env > 0) {
                    psd->iid_index[env][b] = psd->iid_index[env - 1][b] + delta;
                } else if (env == 0) {
                    psd->iid_index[env][b] = psd->iid_index_prev[b] + delta;
                } else {
                    psd->iid_index[env][b] = psd->iid_index[env - 1][b] + delta;
                }
                /* Clip index to [-7, 7] range */
                if (psd->iid_index[env][b] < -7) {
                    psd->iid_index[env][b] = -7;
                }
                if (psd->iid_index[env][b] > 7) {
                    psd->iid_index[env][b] = 7;
                }
            }
        }
        for (b = 0; b < num_subbands; b++) {
            psd->iid_index_prev[b] = psd->iid_index[hdr->num_env - 1][b];
        }
    }

    /* Decode ICC delta codes */
    if (hdr->enable_icc) {
        for (env = 0; env < hdr->num_env; env++) {
            hdr->icc_dt[env] = GetBits(bsi, 1);
            for (b = 0; b < num_subbands; b++) {
                int delta = DecodePSHuffman(bsi, 1); /* Decode ICC Huffman code */
                if (hdr->icc_dt[env] && env > 0) {
                    psd->icc_index[env][b] = psd->icc_index[env - 1][b] + delta;
                } else if (env == 0) {
                    psd->icc_index[env][b] = psd->icc_index_prev[b] + delta;
                } else {
                    psd->icc_index[env][b] = psd->icc_index[env - 1][b] + delta;
                }
                /* Clip index to [0, 7] range */
                if (psd->icc_index[env][b] < 0) {
                    psd->icc_index[env][b] = 0;
                }
                if (psd->icc_index[env][b] > 7) {
                    psd->icc_index[env][b] = 7;
                }
            }
        }
        for (b = 0; b < num_subbands; b++) {
            psd->icc_index_prev[b] = psd->icc_index[hdr->num_env - 1][b];
        }
    }

    return 0;
}

/**************************************************************************************
    Function:    HybridAnalysisFilterbank

    Description: Perform 71/91-band sub-QMF FIR filterbank analysis on band k
 **************************************************************************************/
static void HybridAnalysisFilterbank(PSData *psd, int band, int input[2], int sub_out[12][2], int num_subbands) {
    int j;

    /* Shift history buffer by 1 sample */
    memmove(psd->hybrid_buffer[band][1], psd->hybrid_buffer[band][0], 11 * sizeof(int) * 2);
    psd->hybrid_buffer[band][0][0] = input[0];
    psd->hybrid_buffer[band][0][1] = input[1];

    if (num_subbands == 8) {
        /* 8-channel sub-QMF FIR filter */
        for (int q = 0; q < 8; q++) {
            int acc_re = 0, acc_im = 0;
            for (j = 0; j < 12; j++) {
                int coeff = READ_TAB_INT(p8_13_20, (j < 6) ? j : (12 - j));
                acc_re += MUL_Q30(coeff, psd->hybrid_buffer[band][j][0]);
                acc_im += MUL_Q30(coeff, psd->hybrid_buffer[band][j][1]);
            }
            sub_out[q][0] = acc_re;
            sub_out[q][1] = acc_im;
        }
    } else if (num_subbands == 4) {
        /* 4-channel sub-QMF FIR filter */
        for (int q = 0; q < 4; q++) {
            int acc_re = 0, acc_im = 0;
            for (j = 0; j < 12; j++) {
                int coeff = READ_TAB_INT(p4_13_20, (j < 6) ? j : (12 - j));
                acc_re += MUL_Q30(coeff, psd->hybrid_buffer[band][j][0]);
                acc_im += MUL_Q30(coeff, psd->hybrid_buffer[band][j][1]);
            }
            sub_out[q][0] = acc_re;
            sub_out[q][1] = acc_im;
        }
    } else {
        /* Direct pass-through for 1-to-1 bands */
        sub_out[0][0] = input[0];
        sub_out[0][1] = input[1];
    }
}

/**************************************************************************************
    Function:    ProcessPSSlot

    Description: Apply fixed-point PS mixing matrix & allpass decorrelator to slot l
 **************************************************************************************/
void ProcessPSSlot(PSData *psd, int Xbuf_slot[64][2], int slot_L[64][2], int slot_R[64][2], int l) {
    int k, env;
    PSHeader *hdr = &psd->hdr;

    /* Determine active PS envelope for slot l */
    env = 0;
    for (int e = 0; e < hdr->num_env; e++) {
        if (l >= hdr->border_position[e] && l < hdr->border_position[e + 1]) {
            env = e;
            break;
        }
    }

    int num_param_subbands = (hdr->iid_mode < 3) ? PS_NUM_SUBBANDS_20 : PS_NUM_SUBBANDS_34;

    /* Sub-QMF analysis outputs for bands 0..2 */
    int sub_analysis[3][12][2];
    HybridAnalysisFilterbank(psd, 0, Xbuf_slot[0], sub_analysis[0], 8);
    HybridAnalysisFilterbank(psd, 1, Xbuf_slot[1], sub_analysis[1], 4);
    HybridAnalysisFilterbank(psd, 2, Xbuf_slot[2], sub_analysis[2], 4);

    /* Clear output slots */
    memset(slot_L, 0, 64 * sizeof(int) * 2);
    memset(slot_R, 0, 64 * sizeof(int) * 2);

    /* Subbands 0..31: 71/91 Hybrid Subband Processing */
    int hybrid_idx = 0;
    for (k = 0; k < 32; k++) {
        int num_sub = (k == 0) ? 8 : (k < 3 ? 4 : 1);

        for (int s = 0; s < num_sub; s++) {
            int re, im;
            if (k < 3) {
                re = sub_analysis[k][s][0];
                im = sub_analysis[k][s][1];
            } else {
                re = Xbuf_slot[k][0];
                im = Xbuf_slot[k][1];
            }

            int b = (k < 20) ? k : 20 + ((k - 20) >> 1);
            if (b >= num_param_subbands) {
                b = num_param_subbands - 1;
            }

            int iid_idx = psd->iid_index[env][b] + 7;
            int icc_idx = psd->icc_index[env][b];

            int c1 = READ_TAB_INT(iid_scale_tab, iid_idx);
            int c2 = READ_TAB_INT(iid_scale_tab, 14 - iid_idx);
            int cos_a = READ_TAB_INT(icc_cos_tab, icc_idx);
            int sin_a = READ_TAB_INT(icc_sin_tab, icc_idx);

            /* Fixed-point Q30 PS mixing matrix coefficients */
            int h11 = MUL_Q30(c1, cos_a);
            int h12 = MUL_Q30(c1, sin_a);
            int h21 = MUL_Q30(c2, cos_a);
            int h22 = -MUL_Q30(c2, sin_a);

            /* Smoothly interpolate mixing matrix coefficients over time slot */
            if (l == 0) {
                psd->h11_prev[hybrid_idx] = h11;
                psd->h12_prev[hybrid_idx] = h12;
                psd->h21_prev[hybrid_idx] = h21;
                psd->h22_prev[hybrid_idx] = h22;
            } else {
                h11 = psd->h11_prev[hybrid_idx] + ((h11 - psd->h11_prev[hybrid_idx]) >> 2);
                h12 = psd->h12_prev[hybrid_idx] + ((h12 - psd->h12_prev[hybrid_idx]) >> 2);
                h21 = psd->h21_prev[hybrid_idx] + ((h21 - psd->h21_prev[hybrid_idx]) >> 2);
                h22 = psd->h22_prev[hybrid_idx] + ((h22 - psd->h22_prev[hybrid_idx]) >> 2);
                psd->h11_prev[hybrid_idx] = h11;
                psd->h12_prev[hybrid_idx] = h12;
                psd->h21_prev[hybrid_idx] = h21;
                psd->h22_prev[hybrid_idx] = h22;
            }

            /* Allpass decorrelator stage using alpha_tab coefficient g */
            int g = READ_TAB_INT(alpha_tab, k & 7);

            /* True Allpass filter: w[n] = g * (x[n] - w_prev) + w_prev */
            int w_re = MUL_Q30(g, re - psd->allpass_delay[k][0][0]) + psd->allpass_delay[k][0][0];
            int w_im = MUL_Q30(g, im - psd->allpass_delay[k][0][1]) + psd->allpass_delay[k][0][1];

            psd->allpass_delay[k][0][0] = w_re;
            psd->allpass_delay[k][0][1] = w_im;

            int d_re = w_re;
            int d_im = w_im;

            /* Left channel subband = h11 * S + h12 * D */
            int out_L_re = MUL_Q30(re, h11) + MUL_Q30(d_re, h12);
            int out_L_im = MUL_Q30(im, h11) + MUL_Q30(d_im, h12);

            /* Right channel subband = h21 * S + h22 * D */
            int out_R_re = MUL_Q30(re, h21) + MUL_Q30(d_re, h22);
            int out_R_im = MUL_Q30(im, h21) + MUL_Q30(d_im, h22);

            /* Accumulate / synthesize sub-QMF subbands back into QMF band k */
            slot_L[k][0] += out_L_re;
            slot_L[k][1] += out_L_im;
            slot_R[k][0] += out_R_re;
            slot_R[k][1] += out_R_im;

            hybrid_idx++;
        }
    }

    /* Subbands 32..63: Fast block memcpy passthrough for high frequencies */
    memcpy(&slot_L[32], &Xbuf_slot[32], 32 * sizeof(int) * 2);
    memcpy(&slot_R[32], &Xbuf_slot[32], 32 * sizeof(int) * 2);
}
