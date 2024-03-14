#ifndef MPEG3CSS_H
#define MPEG3CSS_H


#include "mpeg3private.inc"
#include "mpeg3private.h"

int mpeg3_decrypt_packet(mpeg3_css_t *css, unsigned char *sector, int offset);
int mpeg3_delete_css(mpeg3_css_t *css);
int mpeg3_get_keys(mpeg3_css_t *css, char *path);

#endif
