#include "yuv2rgb.h"
#include <inttypes.h>
#include <string.h>

static void         *table_rV[256];
static void         *table_gU[256];
static int           table_gV[256];
static void         *table_bU[256];

#define RGB(i)							\
	U = pu[i];						\
	V = pv[i];						\
	r = table_rV[V];					\
	g = (void *) (((guchar *)table_gU[U]) + table_gV[V]);	\
	b = table_bU[U];

#define DST1RGB(i)							\
	Y = py_1[2*i];							\
	dst_1[6*i] = r[Y]; dst_1[6*i+1] = g[Y]; dst_1[6*i+2] = b[Y];	\
	Y = py_1[2*i+1];						\
	dst_1[6*i+3] = r[Y]; dst_1[6*i+4] = g[Y]; dst_1[6*i+5] = b[Y];

#define DST2RGB(i)							\
	Y = py_2[2*i];							\
	dst_2[6*i] = r[Y]; dst_2[6*i+1] = g[Y]; dst_2[6*i+2] = b[Y];	\
	Y = py_2[2*i+1];						\
	dst_2[6*i+3] = r[Y]; dst_2[6*i+4] = g[Y]; dst_2[6*i+5] = b[Y];

void yuv2rgb (guchar *_pyuv,
	      guchar * _dst, gint width, gint height) {

  guchar *_py, *_pu, *_pv;
  gint    U, V, Y;
  guchar *py_1, *py_2, *pu, *pv;
  guchar *r, *g, *b;
  guchar *dst_1, *dst_2;
  gint    rgb_stride, y_stride, uv_stride;
  gint    source_width, source_height;

  _py = _pyuv;
  _pv = _pyuv + width*height;
  _pu = _pyuv + width*height*5/4;

  source_width = width;
  source_height = height;

  height = source_height >> 1;
  rgb_stride = width*3;
  y_stride   = width;
  uv_stride  = width/2;
  do {
    dst_1 = _dst;
    dst_2 = (void*)( (guchar *)_dst + rgb_stride );
    py_1  = _py;
    py_2  = _py + y_stride;
    pu    = _pu;
    pv    = _pv;
    
    width = source_width >> 3;
    do {
      RGB(0);
      DST1RGB(0);
      DST2RGB(0);
      
      RGB(1);
      DST2RGB(1);
      DST1RGB(1);
      
      RGB(2);
      DST1RGB(2);
      DST2RGB(2);
      
      RGB(3);
      DST2RGB(3);
      DST1RGB(3);
      
      pu += 4;
      pv += 4;
      py_1 += 8;
      py_2 += 8;
      dst_1 += 24;
      dst_2 += 24;
    } while (--width);
    
    _dst += 2 * rgb_stride; 
    _py += 2 * y_stride;
    _pu += uv_stride;
    _pv += uv_stride;
    
  } while (--height);
  
}

const int32_t Inverse_Table_6_9[8][4] = {
  {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
  {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
  {104597, 132201, 25675, 53279}, /* unspecified */
  {104597, 132201, 25675, 53279}, /* reserved */
  {104448, 132798, 24759, 53109}, /* FCC */
  {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
  {104597, 132201, 25675, 53279}, /* SMPTE 170M */
  {117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
};

static int div_round (int dividend, int divisor)
{
  if (dividend > 0)
    return (dividend + (divisor>>1)) / divisor;
  else
    return -((-dividend + (divisor>>1)) / divisor);
}

void yuv2rgb_init () {


  int i;
  uint8_t table_Y[1024];
  uint8_t * table_8 = 0;
  int entry_size = 0;
  void *table_r = 0, *table_g = 0, *table_b = 0;

  int crv = Inverse_Table_6_9[6][0];
  int cbu = Inverse_Table_6_9[6][1];
  int cgu = -Inverse_Table_6_9[6][2];
  int cgv = -Inverse_Table_6_9[6][3];

  for (i = 0; i < 1024; i++) {
    int j;

    j = (76309 * (i - 384 - 16) + 32768) >> 16;
    j = (j < 0) ? 0 : ((j > 255) ? 255 : j);
    table_Y[i] = j;
  }

  table_8 = malloc ((256 + 2*232) * sizeof (uint8_t));

  entry_size = sizeof (uint8_t);
  table_r = table_g = table_b = table_8 + 232;
  
  for (i = -232; i < 256+232; i++)
    ((uint8_t * )table_b)[i] = table_Y[i+384];
  
  
  for (i = 0; i < 256; i++) {
    table_rV[i] = (((guchar *) table_r) +
		   entry_size * div_round (crv * (i-128), 76309));
    table_gU[i] = (((guchar *) table_g) +
		   entry_size * div_round (cgu * (i-128), 76309));
    table_gV[i] = entry_size * div_round (cgv * (i-128), 76309);
    table_bU[i] = (((guchar *)table_b) +
		   entry_size * div_round (cbu * (i-128), 76309));
  }
}



