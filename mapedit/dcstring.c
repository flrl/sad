#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "mapedit/dcstring.h"

#define DCSTRING_ALLOC  (64) /* increase allocation in chunks of this size */

#define DC_PBEFORE(dc)  ((dc)->buffer)
#define DC_PGAP(dc)     (&(dc)->buffer[(dc)->len_before])
#define DC_PAFTER(dc)   (&(dc)->buffer[(dc)->size - ((dc)->len_after + 1)])

#ifndef NDEBUG
#define DC_SANITY(_dc)                                                  \
do {                                                                    \
    const dcstring *__dc = (_dc);                                       \
    assert((   __dc->buffer == NULL && __dc->size == 0                  \
            && __dc->len_before == 0 && __dc->len_gap == 0              \
            && __dc->len_after == 0)                                    \
           ||                                                           \
           (   __dc->buffer != NULL                                     \
            && __dc->len_before + __dc->len_gap + __dc->len_after + 1   \
               == __dc->size));                                         \
} while (0)
#else
#define DC_SANITY(_dc) ((void)(0))
#endif

struct s_dcstring {
    char *buffer;
    size_t size;
    size_t len_before;
    size_t len_gap;
    size_t len_after;
};

static int dcstring_fit(dcstring *dc, size_t len)
{
    assert(dc != NULL);
    DC_SANITY(dc);

    if (!dc->buffer || !dc->size) {
        dc->buffer = malloc(DCSTRING_ALLOC);
        if (!dc->buffer) return -1;
        dc->size = DCSTRING_ALLOC;
        memset(dc->buffer, 0, dc->size);
        dc->len_before = dc->len_after = 0;
        dc->len_gap = dc->size - 1;
        return 0;
    }
    else if (dc->len_gap > len) {
        /* already enough space */
        return 0;
    }
    else {
        size_t incr = DCSTRING_ALLOC;
        char *tmp;

        while (dc->len_gap + incr < len)
            incr += DCSTRING_ALLOC;

        tmp = malloc(dc->size + incr);
        if (!tmp) return -1;

        memset(tmp, 0, dc->size + incr);
        if (dc->len_before)
            memcpy(tmp, dc->buffer, dc->len_before);
        if (dc->len_after)
            memcpy(&tmp[(dc->size + incr) - (dc->len_after + 1)],
                   DC_PAFTER(dc),
                   dc->len_after);

        free(dc->buffer);
        dc->buffer = tmp;
        dc->size += incr;
        dc->len_gap += incr;

        return 0;
    }
}

dcstring *dcstring_new(const char *initial)
{
    dcstring *dc = malloc(sizeof *dc);
    if (!dc) return NULL;

    memset(dc, 0, sizeof *dc);

    if (initial) {
        if (dcstring_sinsert(dc, initial) < 0) {
            dcstring_free(&dc);
            return NULL;
        }
    }

    DC_SANITY(dc);
    return dc;
}

dcstring *dcstring_clone(const dcstring *dc)
{
    if (!dc) return NULL;
    DC_SANITY(dc);

    dcstring *clone = malloc(sizeof *clone);
    if (!clone) return NULL;

    if (!dc->size) {
        memset(clone, 0, sizeof *clone);
        return clone;
    }

    clone->buffer = malloc(dc->size);
    if (!clone->buffer) {
        free(clone);
        return NULL;
    }

    clone->size = dc->size;
    clone->len_before = dc->len_before;
    clone->len_gap = dc->len_gap;
    clone->len_after = dc->len_after;

    if (dc->len_before)
        memcpy(DC_PBEFORE(clone), DC_PBEFORE(dc), dc->len_before);
    if (dc->len_after)
        memcpy(DC_PAFTER(clone), DC_PAFTER(dc), dc->len_after);
    memset(DC_PGAP(clone), 0, dc->len_gap);

    DC_SANITY(clone);
    return clone;
}

void dcstring_free(dcstring **dcp)
{
    dcstring *dc = *dcp;
    *dcp = NULL;

    if (dc->buffer) free(dc->buffer);
    memset(dc, 0, sizeof *dc);
    free(dc);
}

size_t dcstring_len_before(const dcstring *dc)
{
    DC_SANITY(dc);
    return dc->len_before;
}

size_t dcstring_len_after(const dcstring *dc)
{
    DC_SANITY(dc);
    return dc->len_after;
}

size_t dcstring_len(const dcstring *dc)
{
    DC_SANITY(dc);
    return dc->len_before + dc->len_after;
}

const char *dcstring_before(const dcstring *dc)
{
    DC_SANITY(dc);
    if (!dc->size || !dc->buffer) return NULL;
    return DC_PBEFORE(dc);
}

const char *dcstring_after(const dcstring *dc)
{
    DC_SANITY(dc);
    if (!dc->size || !dc->buffer) return NULL;
    return DC_PAFTER(dc);
}

char *dcstring_strdup(const dcstring *dc, size_t *out_len)
{
    DC_SANITY(dc);
    char *s, *p;

    s = malloc(dc->len_before + dc->len_after + 1);
    if (!s) return NULL;

    p = s;
    if (dc->len_before) {
        memcpy(p, DC_PBEFORE(dc), dc->len_before);
        p += dc->len_before;
    }
    if (dc->len_after) {
        memcpy(p, DC_PAFTER(dc), dc->len_after);
        p += dc->len_after;
    }
    *p = '\0';

    if (out_len) *out_len = dc->len_before + dc->len_after;
    return s;
}

int dcstring_cinsert(dcstring *dc, int c)
{
    if (dcstring_fit(dc, 1) < 0) return -1;
    DC_SANITY(dc);

    dc->buffer[dc->len_before++] = c;
    return 0;
}

int dcstring_ninsert(dcstring *dc, size_t len, const char *s)
{
    if (!len) return 0;
    if (dcstring_fit(dc, len) < 0) return -1;
    DC_SANITY(dc);

    memmove(DC_PGAP(dc), s, len);
    dc->len_before += len;

    return 0;
}

ptrdiff_t dcstring_ndelete(dcstring *dc, ptrdiff_t count)
{
    DC_SANITY(dc);

    if (count > 0) {
        count = (size_t) count < dc->len_after ? count : dc->len_after;
        if (count == 0) return 0;

        memset(DC_PAFTER(dc), 0, count);
        dc->len_after -= count;
        dc->len_gap += count;
        return count;
    }
    else if (count < 0) {
        count = (size_t) -count < dc->len_before ? -count : dc->len_before;
        if (count == 0) return 0;

        memset(DC_PGAP(dc) - count, 0, count);
        dc->len_before -= count;
        dc->len_gap += count;
        return count;
    }
    else {
        return 0;
    }
}

void dcstring_cursor_left(dcstring *dc)
{
    DC_SANITY(dc);
    if (dc->len_before == 0) return;

    *(DC_PAFTER(dc) - 1) = *(DC_PGAP(dc) - 1);
    dc->len_before--;
    dc->len_after++;
}

void dcstring_cursor_right(dcstring *dc)
{
    DC_SANITY(dc);
    if (dc->len_after == 0) return;
    *DC_PGAP(dc) = *DC_PAFTER(dc);
    dc->len_before++;
    dc->len_after--;
}

ptrdiff_t dcstring_cursor_move(dcstring *dc, ptrdiff_t by)
{
    DC_SANITY(dc);
    if (by > 0) {
        by = (size_t) by < dc->len_after ? by : dc->len_after;
        if (by == 0) return 0;
        memmove(DC_PGAP(dc), DC_PAFTER(dc), by);
        dc->len_before += by;
        dc->len_after  -= by;
        memset(DC_PGAP(dc), 0, by);
        return by;
    }
    else if (by < 0) {
        by = (size_t) -by < dc->len_before ? -by : dc->len_before;
        if (by == 0) return 0;
        memmove(DC_PAFTER(dc) - by, DC_PGAP(dc) - by, by);
        dc->len_before -= by;
        dc->len_after += by;
        memset(DC_PGAP(dc), 0, by);
        return -by;
    }
    else {
        return 0;
    }
}

ptrdiff_t dcstring_cursor_tostart(dcstring *dc)
{
    DC_SANITY(dc);
    if (dc->len_before == 0) return 0;
    ptrdiff_t moved = 0 - dc->len_before;

    memcpy(DC_PAFTER(dc) - dc->len_before, DC_PBEFORE(dc), dc->len_before);
    dc->len_after += dc->len_before;
    dc->len_before = 0;
    memset(DC_PGAP(dc), 0, dc->len_gap);

    return moved;
}

ptrdiff_t dcstring_cursor_toend(dcstring *dc)
{
    DC_SANITY(dc);
    if (dc->len_after == 0) return 0;
    ptrdiff_t moved = dc->len_after;

    memcpy(DC_PGAP(dc), DC_PAFTER(dc), dc->len_after);
    dc->len_before += dc->len_after;
    dc->len_after = 0;
    memset(DC_PGAP(dc), 0, dc->len_gap);

    return moved;
}
