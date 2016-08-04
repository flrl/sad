#ifndef MAPEDIT_DCSTRING_H
#define MAPEDIT_DCSTRING_H

#include <stddef.h>

typedef struct s_dcstring dcstring;

dcstring *dcstring_new(const char *initial);
dcstring *dcstring_clone(const dcstring *dc);
char *dcstring_release(dcstring **dcp);
void dcstring_free(dcstring **dcp);

size_t dcstring_len_before(const dcstring *dc);
size_t dcstring_len_after(const dcstring *dc);
size_t dcstring_len(const dcstring *dc);

const char *dcstring_before(const dcstring *dc);
const char *dcstring_after(const dcstring *dc);
char *dcstring_strdup(const dcstring *dc, size_t *out_len);

int dcstring_cinsert(dcstring *dc, int c);
int dcstring_ninsert(dcstring *dc, size_t len, const char *s);
#define dcstring_sinsert(dc,s) ((s) ? dcstring_ninsert((dc), strlen((s)), (s)) : 0)

ptrdiff_t dcstring_ndelete(dcstring *dc, ptrdiff_t count);

void dcstring_cursor_left(dcstring *dc);
void dcstring_cursor_right(dcstring *dc);
ptrdiff_t dcstring_cursor_move(dcstring *dc, ptrdiff_t by);
ptrdiff_t dcstring_cursor_tostart(dcstring *dc);
ptrdiff_t dcstring_cursor_toend(dcstring *dc);

#endif
