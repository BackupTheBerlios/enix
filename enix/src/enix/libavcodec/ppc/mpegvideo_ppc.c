/*
 * Copyright (c) 2002 Dieter Shirley
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include <time.h>
#include "../../config.h"
#include "../dsputil.h"
#include "../mpegvideo.h"

#ifdef HAVE_ALTIVEC
#include "dsputil_altivec.h"
#endif

extern int dct_quantize_altivec(MpegEncContext *s,  
        DCTELEM *block, int n,
        int qscale, int *overflow);

extern void idct_put_altivec(UINT8 *dest, int line_size, INT16 *block);
extern void idct_add_altivec(UINT8 *dest, int line_size, INT16 *block);


void MPV_common_init_ppc(MpegEncContext *s)
{
#if HAVE_ALTIVEC
    if (has_altivec())
    {
        if ((s->avctx->idct_algo == FF_IDCT_AUTO) ||
                (s->avctx->idct_algo == FF_IDCT_ALTIVEC))
        {
            s->idct_put = idct_put_altivec;
            s->idct_add = idct_add_altivec;
            s->idct_permutation_type = FF_TRANSPOSE_IDCT_PERM;
        }

        // Test to make sure that the dct required alignments are met.
        if ((((long)(s->q_intra_matrix) & 0x0f) != 0) ||
                (((long)(s->q_inter_matrix) & 0x0f) != 0))
        {
            fprintf(stderr, "Internal Error: q-matrix blocks must be 16-byte aligned "
                    "to use Altivec DCT. Reverting to non-altivec version.\n");
            return;
        }

        if (((long)(s->intra_scantable.inverse) & 0x0f) != 0)
        {
            fprintf(stderr, "Internal Error: scan table blocks must be 16-byte aligned "
                    "to use Altivec DCT. Reverting to non-altivec version.\n");
            return;
        }


        if ((s->avctx->dct_algo == FF_DCT_AUTO) ||
                (s->avctx->dct_algo == FF_DCT_ALTIVEC))
        {
            s->dct_quantize = dct_quantize_altivec;
        }
    } else
#endif
    {
        /* Non-AltiVec PPC optimisations here */
    }
}

