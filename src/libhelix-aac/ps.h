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

#define EXT_PS          2

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
    int num_env;
    int border_position[MAX_PS_ENVELOPES + 1];
    int iid_dt[MAX_PS_ENVELOPES];
    int icc_dt[MAX_PS_ENVELOPES];
} PSHeader;

typedef struct _PSData {
    PSHeader hdr;
    int iid_index[MAX_PS_ENVELOPES][PS_NUM_SUBBANDS_34];
    int icc_index[MAX_PS_ENVELOPES][PS_NUM_SUBBANDS_34];
    int iid_index_prev[PS_NUM_SUBBANDS_34];
    int icc_index_prev[PS_NUM_SUBBANDS_34];
    int h11[MAX_PS_ENVELOPES][PS_NUM_SUBBANDS_34];
    int h12[MAX_PS_ENVELOPES][PS_NUM_SUBBANDS_34];
    int h21[MAX_PS_ENVELOPES][PS_NUM_SUBBANDS_34];
    int h22[MAX_PS_ENVELOPES][PS_NUM_SUBBANDS_34];
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
