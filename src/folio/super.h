#ifndef SUPER_H
#define SUPER_H

#include "types.h"
#include "folio.h"

#define SUPER_FOLIO 'S'
#define SUPER_FOLIO_SWI (SUPER_FOLIO << 16)

#define SWI_SUPER   (SUPER_FOLIO_SWI + 0)

void __swi(SWI_SUPER) Super(void (*func)(void));

#endif