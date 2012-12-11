/* A header file for the AdaptiveCounter implementation. */

#include "postgres.h"

/* This is an implementation of Adaptive Sampling algorithm presented in
 * paper "On Adaptive Sampling" published in 1990 (written by P. Flajolet).
 */
typedef struct AdaptiveCounterData {
  
    /* length of this counter (including the whole bitmap) */
    /* Quote from http://www.postgresql.org/docs/9.2/static/xfunc-c.html:
     * "All variable-length types must begin with an opaque length field of exactly 4 bytes,
     * which will be set by SET_VARSIZE; never set this field directly!" */
    int4 length;
    
    /* maximal number of items in the list */
    int maxItems;
    
    /* number of bytes to store for each item (default is the whole hash) */
    /* It is not necessary to store complete hashes (16B => 2^128 values,
     * which is usually much more than maxItems). 2^32 or 2^40 should be just
     * fine in most cases, reducing the space for list considerably). */
    int itemSize;
    
    /* expected number of distinct values */
    int ndistinct;
    
    /* current number of items in the list */
    int items;
    
    /* current level */
    int level;
    
    /* error rate */
    float error;
    
    /* bitmap used to keep the list of items (uses the very same trick as in
     * the varlena type in include/c.h */
    unsigned char bitmap[1];
    
} AdaptiveCounterData;

typedef AdaptiveCounterData* AdaptiveCounter;

/* Creates a counter based on adaptive sampling, with a given error rate */
AdaptiveCounter ac_init(float error, int ndistinct);

/* Reset the counter (so that it seems to e empty) */
void ac_reset(AdaptiveCounter ac);

/* Copy the data to a completely new */
AdaptiveCounter ac_create_copy(AdaptiveCounter src);

/* add element into the counter */
void ac_add_item_text(AdaptiveCounter ac, const char * element, int elen);
void ac_add_item_int(AdaptiveCounter ac, int element);

/* print info about the counter */
void ac_print_info(AdaptiveCounter ac);

/* get estimate */
int ac_estimate(AdaptiveCounter ac);

/* merge two adaptive counters */
AdaptiveCounter ac_merge(AdaptiveCounter dest, AdaptiveCounter src);
