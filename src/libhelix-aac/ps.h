/* ***** BEGIN LICENSE BLOCK *****
    Source last modified: $Id: ps.h,v 1.0 2026/10/09 00:00:00 $

    Portions Copyright (c) RealNetworks, Inc. All Rights Reserved.
    Portions Copyright (C) 2026 Nils Schimmelmann
 * ***** END LICENSE BLOCK ***** */

#ifndef _PS_H
#define _PS_H

#include "aaccommon.h"
#include "bitstream.h"
#include <pgmspace.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXT_PS             2

#define MAX_PS_ENVELOPES   5
#define PS_NUM_SUBBANDS_20 20
#define PS_NUM_SUBBANDS_34 34

typedef struct _PSHeader {
    int enable_ps_header;
    int enable_iid;
    int iid_mode;
    int enable_icc;
    int icc_mode;
    int enable_ext;
    int enable_ipdopd;
    int ipd_mode;
    int use34hybrid_bands;
    int num_env;
    int border_position[MAX_PS_ENVELOPES + 1];
    int iid_dt[MAX_PS_ENVELOPES];
    int icc_dt[MAX_PS_ENVELOPES];
    int ipd_dt[MAX_PS_ENVELOPES];
    int opd_dt[MAX_PS_ENVELOPES];
} PSHeader;

typedef struct _PSData {
    PSHeader hdr;
    int iid_index[MAX_PS_ENVELOPES][34];
    int icc_index[MAX_PS_ENVELOPES][34];
    int ipd_index[MAX_PS_ENVELOPES][17];
    int opd_index[MAX_PS_ENVELOPES][17];
    int iid_index_prev[34];
    int icc_index_prev[34];
    int ipd_index_prev[17];
    int opd_index_prev[17];

    /* Hybrid analysis filterbank history */
    int hybrid_work[12 + 32][2];
    int hybrid_buffer[5][12][2];

    /* Decorrelator and delay buffers */
    int delay_buf_index_delay[64];
    int delay_buf_index_ser[3];
    int saved_delay;
    int delay_Qmf[2][64][2];
    int delay_SubQmf[2][32][2];
    int delay_Qmf_ser[3][5][64][2];
    int delay_SubQmf_ser[3][5][32][2];

    /* Transient energy state */
    int P_PeakDecayNrg[34];
    int P_SmoothPeakDecayDiffNrg_prev[34];
    int P_prev[34];

    /* Mixing matrix interpolation state */
    int h11_prev[64];
    int h12_prev[64];
    int h21_prev[64];
    int h22_prev[64];

    /* Allpass state */
    int allpass_delay[32][2][2];
} PSData;

extern const int iid_scale_tab[15] PROGMEM;
extern const int icc_cos_tab[8] PROGMEM;
extern const int icc_sin_tab[8] PROGMEM;
extern const int alpha_tab[8] PROGMEM;

int DecodePSHeader(BitStreamInfo *bsi, PSHeader *hdr);
int DecodePSHuffman(BitStreamInfo *bsi, int type);
int DecodePSDataPayload(BitStreamInfo *bsi, PSData *psd);
void ProcessPSSlot(PSData *psd, int Xbuf_slot[64][2], int slot_L[64][2], int slot_R[64][2], int l);

#ifdef __cplusplus
}
#endif

#endif /* _PS_H */
