
/* predict.h, Low-level Architecture neutral prediction
 * (motion compensated reconstruction) routines */

/*  (C) 2003 Andrew Stevens */

/* These modifications are free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#ifndef _PREDICT_H
#define _PREDICT_H

#ifdef  __cplusplus
extern "C" {
#endif

    extern void (*ppred_comp)( uint8_t *src, uint8_t *dst,
                               int stride, int w, int h, int x, int y, int dx, int dy,
                               int addflag);
    
    void pred_comp( uint8_t *src, uint8_t *dst,
                    int stride, int w, int h, int x, int y, int dx, int dy,
                    int addflag);
    

    void clearblock ( uint8_t *cur[3], int i0, int j0, 
                  int field_off,
                  int stride);

    void init_predict(void);


#ifdef  __cplusplus
}
#endif

#endif /* _PREDICT_H */


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */

