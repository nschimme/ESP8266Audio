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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXT_PS             2

#define MAX_PS_ENVELOPES   5
#define PS_NUM_SUBBANDS_20 20
#define PS_NUM_SUBBANDS_34 34
#define PS_MAX_HYBRID_BANDS 91

typedef struct _PSHeader {
    uint8_t enable_ps_header;
    uint8_t enable_iid;
    uint8_t iid_mode;
    uint8_t enable_icc;
    uint8_t icc_mode;
    uint8_t enable_ext;
    uint8_t enable_ipdopd;
    uint8_t ipd_mode;
    uint8_t use34hybrid_bands;
    uint8_t num_env;
    uint8_t border_position[MAX_PS_ENVELOPES + 1];
    int8_t  iid_dt[MAX_PS_ENVELOPES];
    int8_t  icc_dt[MAX_PS_ENVELOPES];
    int8_t  ipd_dt[MAX_PS_ENVELOPES];
    int8_t  opd_dt[MAX_PS_ENVELOPES];
} PSHeader;

typedef struct _PSData {
    PSHeader hdr;
    int8_t  iid_index[MAX_PS_ENVELOPES][34];
    int8_t  icc_index[MAX_PS_ENVELOPES][34];
    int8_t  ipd_index[MAX_PS_ENVELOPES][17];
    int8_t  opd_index[MAX_PS_ENVELOPES][17];
    int8_t  iid_index_prev[34];
    int8_t  icc_index_prev[34];
    int8_t  ipd_index_prev[17];
    int8_t  opd_index_prev[17];

    /* 71/91-band Hybrid analysis filterbank history (12 delay taps per band) */
    int hybrid_buffer[5][12][2];

    /* Decorrelator and delay buffers */
    uint8_t delay_buf_index_delay[64];
    uint8_t delay_buf_index_ser[3];
    uint8_t saved_delay;
    int delay_Qmf[2][64][2];
    int delay_SubQmf[2][32][2];
    int delay_Qmf_ser[3][5][64][2];
    int delay_SubQmf_ser[3][5][32][2];

    /* Transient energy state */
    int P_PeakDecayNrg[34];
    int P_SmoothPeakDecayDiffNrg_prev[34];
    int P_prev[34];

    /* Mixing matrix interpolation state for 91 subbands */
    int h11_prev[PS_MAX_HYBRID_BANDS];
    int h12_prev[PS_MAX_HYBRID_BANDS];
    int h21_prev[PS_MAX_HYBRID_BANDS];
    int h22_prev[PS_MAX_HYBRID_BANDS];

    /* Allpass state */
    int allpass_delay[32][2][2];
} PSData;

extern const int iid_scale_tab[15] PROGMEM;
extern const int icc_cos_tab[8] PROGMEM;
extern const int icc_sin_tab[8] PROGMEM;
extern const int alpha_tab[8] PROGMEM;
extern const int p8_13_20[7] PROGMEM;
extern const int p4_13_20[7] PROGMEM;
extern const int p2_13_20[7] PROGMEM;

int DecodePSHeader(BitStreamInfo *bsi, PSHeader *hdr);
int DecodePSHuffman(BitStreamInfo *bsi, int type);
int DecodePSDataPayload(BitStreamInfo *bsi, PSData *psd);
void ProcessPSSlot(PSData *psd, int Xbuf_slot[64][2], int slot_L[64][2], int slot_R[64][2], int l);

#ifdef __cplusplus
}
#endif

#endif /* _PS_H */
