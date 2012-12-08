#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>

#include "postgres.h"
#include "fmgr.h"
#include "adaptive.h"
#include "utils/builtins.h"
#include "utils/bytea.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define VAL(CH)			((CH) - '0')
#define DIG(VAL)		((VAL) + '0')

#define DEFAULT_ERROR       0.025
#define DEFAULT_NDISTINCT   1000000

PG_FUNCTION_INFO_V1(adaptive_add_item_text);
PG_FUNCTION_INFO_V1(adaptive_add_item_int);

PG_FUNCTION_INFO_V1(adaptive_add_item_agg_text);
PG_FUNCTION_INFO_V1(adaptive_add_item_agg_int);

PG_FUNCTION_INFO_V1(adaptive_add_item_agg2_text);
PG_FUNCTION_INFO_V1(adaptive_add_item_agg2_int);

PG_FUNCTION_INFO_V1(adaptive_get_estimate);
PG_FUNCTION_INFO_V1(adaptive_get_ndistinct);
PG_FUNCTION_INFO_V1(adaptive_size);
PG_FUNCTION_INFO_V1(adaptive_init);
PG_FUNCTION_INFO_V1(adaptive_get_error);
PG_FUNCTION_INFO_V1(adaptive_get_item_size);
PG_FUNCTION_INFO_V1(adaptive_reset);
PG_FUNCTION_INFO_V1(adaptive_merge);
PG_FUNCTION_INFO_V1(adaptive_in);
PG_FUNCTION_INFO_V1(adaptive_out);
PG_FUNCTION_INFO_V1(adaptive_rect);
PG_FUNCTION_INFO_V1(adaptive_send);
PG_FUNCTION_INFO_V1(adaptive_length);

Datum adaptive_add_item_text(PG_FUNCTION_ARGS);
Datum adaptive_add_item_int(PG_FUNCTION_ARGS);

Datum adaptive_add_item_agg_text(PG_FUNCTION_ARGS);
Datum adaptive_add_item_agg_int(PG_FUNCTION_ARGS);

Datum adaptive_add_item_agg2_text(PG_FUNCTION_ARGS);
Datum adaptive_add_item_agg2_int(PG_FUNCTION_ARGS);

Datum adaptive_get_estimate(PG_FUNCTION_ARGS);
Datum adaptive_get_ndistinct(PG_FUNCTION_ARGS);
Datum adaptive_size(PG_FUNCTION_ARGS);
Datum adaptive_init(PG_FUNCTION_ARGS);
Datum adaptive_get_error(PG_FUNCTION_ARGS);
Datum adaptive_get_item_size(PG_FUNCTION_ARGS);
Datum adaptive_reset(PG_FUNCTION_ARGS);
Datum adaptive_merge(PG_FUNCTION_ARGS);
Datum adaptive_in(PG_FUNCTION_ARGS);
Datum adaptive_out(PG_FUNCTION_ARGS);
Datum adaptive_recv(PG_FUNCTION_ARGS);
Datum adaptive_send(PG_FUNCTION_ARGS);
Datum adaptive_length(PG_FUNCTION_ARGS);

Datum
adaptive_add_item_text(PG_FUNCTION_ARGS)
{

    AdaptiveCounter ac;
    text * item;
    
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if ((! PG_ARGISNULL(0)) && (! PG_ARGISNULL(1))) {

      ac = (AdaptiveCounter)PG_GETARG_BYTEA_P(0);
      
      /* get the new item */
      item = PG_GETARG_TEXT_P(1);
    
      /* in-place update works only if executed as aggregate */
      ac_add_item_text(ac, VARDATA(item), VARSIZE(item) - VARHDRSZ);
      
    } else if (PG_ARGISNULL(0)) {
        elog(ERROR, "adaptive counter must not be NULL");
    }
    
    PG_RETURN_VOID();

}

Datum
adaptive_add_item_int(PG_FUNCTION_ARGS)
{

    AdaptiveCounter ac;
    int item;
    
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if ((! PG_ARGISNULL(0)) && (! PG_ARGISNULL(1))) {

        ac = (AdaptiveCounter)PG_GETARG_BYTEA_P(0);
        
        /* get the new item */
        item = PG_GETARG_INT32(1);
        
        /* in-place update works only if executed as aggregate */
        ac_add_item_int(ac, item);
      
    } else if (PG_ARGISNULL(0)) {
        elog(ERROR, "adaptive counter must not be NULL");
    }
    
    PG_RETURN_VOID();

}

Datum
adaptive_add_item_agg_text(PG_FUNCTION_ARGS)
{
	
    AdaptiveCounter ac;
    text * item;
    float4 errorRate; /* 0 - 1, e.g. 0.01 means 1% */
    int    ndistinct; /* expected number of distinct values */
  
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if (PG_ARGISNULL(0)) {
      errorRate = PG_GETARG_FLOAT4(2);
      ndistinct = PG_GETARG_INT32(3);
      
      /* ndistinct has to be positive, error rate between 0 and 1 (not 0) */
      if (ndistinct < 1) {
          elog(ERROR, "ndistinct (expected number of distinct values) has to at least 1");
      } else if ((errorRate <= 0) || (errorRate > 1)) {
          elog(ERROR, "error rate has to be between 0 and 1");
      }
      
      ac = ac_init(errorRate, ndistinct);
    } else {
      ac = (AdaptiveCounter)PG_GETARG_BYTEA_P(0);
    }
      
    /* get the new item */
    item = PG_GETARG_TEXT_P(1);
    
    /* in-place update works only if executed as aggregate */
    ac_add_item_text(ac, VARDATA(item), VARSIZE(item) - VARHDRSZ);
    
    /* return the updated bytea */
    PG_RETURN_BYTEA_P(ac);
    
}

Datum
adaptive_add_item_agg_int(PG_FUNCTION_ARGS)
{
    
    AdaptiveCounter ac;
    int item;
    float4 errorRate; /* 0 - 1, e.g. 0.01 means 1% */
    int    ndistinct; /* expected number of distinct values */
  
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if (PG_ARGISNULL(0)) {
      errorRate = PG_GETARG_FLOAT4(2);
      ndistinct = PG_GETARG_INT32(3);
      
      /* ndistinct has to be positive, error rate between 0 and 1 (not 0) */
      if (ndistinct < 1) {
          elog(ERROR, "ndistinct (expected number of distinct values) has to at least 1");
      } else if ((errorRate <= 0) || (errorRate > 1)) {
          elog(ERROR, "error rate has to be between 0 and 1");
      }
      
      ac = ac_init(errorRate, ndistinct);
    } else {
      ac = (AdaptiveCounter)PG_GETARG_BYTEA_P(0);
    }
      
    /* get the new item */
    item = PG_GETARG_INT32(1);
    
    /* in-place update works only if executed as aggregate */
    ac_add_item_int(ac, item);
    
    /* return the updated bytea */
    PG_RETURN_BYTEA_P(ac);
    
}

Datum
adaptive_add_item_agg2_text(PG_FUNCTION_ARGS)
{
    
    AdaptiveCounter ac;
    text * item;
  
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if (PG_ARGISNULL(0)) {
      ac = ac_init(DEFAULT_ERROR, DEFAULT_NDISTINCT);
    } else {
      ac = (AdaptiveCounter)PG_GETARG_BYTEA_P(0);
    }
      
    /* get the new item */
    item = PG_GETARG_TEXT_P(1);
    
    /* in-place update works only if executed as aggregate */
    ac_add_item_text(ac, VARDATA(item), VARSIZE(item) - VARHDRSZ);
    
    /* return the updated bytea */
    PG_RETURN_BYTEA_P(ac);
    
}

Datum
adaptive_add_item_agg2_int(PG_FUNCTION_ARGS)
{
    
    AdaptiveCounter ac;
    int item;
  
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if (PG_ARGISNULL(0)) {
      ac = ac_init(DEFAULT_ERROR, DEFAULT_NDISTINCT);
    } else {
      ac = (AdaptiveCounter)PG_GETARG_BYTEA_P(0);
    }
      
    /* get the new item */
    item = PG_GETARG_INT32(1);
    
    /* in-place update works only if executed as aggregate */
    ac_add_item_int(ac, item);
    
    /* return the updated bytea */
    PG_RETURN_BYTEA_P(ac);
    
}

Datum
adaptive_get_estimate(PG_FUNCTION_ARGS)
{
  
    int estimate;
    AdaptiveCounter ac = (AdaptiveCounter)PG_GETARG_BYTEA_P(0);
    
    /* in-place update works only if executed as aggregate */
    estimate = ac_estimate(ac);
    
    /* return the updated bytea */
    PG_RETURN_FLOAT4(estimate);

}

Datum
adaptive_get_ndistinct(PG_FUNCTION_ARGS)
{
  
    AdaptiveCounter ac = (AdaptiveCounter)PG_GETARG_BYTEA_P(0);
  
    /* return the updated bytea */
    PG_RETURN_INT32(ac->ndistinct);

}

Datum
adaptive_size(PG_FUNCTION_ARGS)
{

    float error;
    int itemSize;
    int maxItems;
    int ndistinct;
    int size, sizeA, sizeB;
      
    error = PG_GETARG_FLOAT4(0);
    ndistinct = PG_GETARG_INT32(1);
      
    /* ndistinct has to be positive, error rate between 0 and 1 (not 0) */
    if (ndistinct < 1) {
        elog(ERROR, "ndistinct (expected number of distinct values) has to at least 1");
    } else if ((error <= 0) || (error > 1)) {
        elog(ERROR, "error rate has to be between 0 and 1");
    }
      
    maxItems = ceil(powf(1.2/error, 2));
    
    sizeA = ceilf((logf(ndistinct / maxItems) / logf(2)) / 8) + 1;
    sizeB = ceilf(logf(ndistinct) / logf(256)) + 1;
    
    itemSize = (sizeA > sizeB) ? sizeA : sizeB;
  
    /* store the length too (including the length field) */
    size = sizeof(AdaptiveCounterData) + (itemSize * maxItems) - 1;

    PG_RETURN_INT32(size);

}

Datum
adaptive_init(PG_FUNCTION_ARGS)
{
      AdaptiveCounter ac;
      float errorRate;
      int ndistinct;
      
      errorRate = PG_GETARG_FLOAT4(0);
      ndistinct = PG_GETARG_INT32(1);
      
      /* ndistinct has to be positive, error rate between 0 and 1 (not 0) */
      if (ndistinct < 1) {
          elog(ERROR, "ndistinct (expected number of distinct values) has to at least 1");
      } else if ((errorRate <= 0) || (errorRate > 1)) {
          elog(ERROR, "error rate has to be between 0 and 1");
      }
      
      ac = ac_init(errorRate, ndistinct);
      
      PG_RETURN_BYTEA_P(ac);
}

Datum
adaptive_length(PG_FUNCTION_ARGS)
{
    PG_RETURN_INT32(VARSIZE((AdaptiveCounter)PG_GETARG_BYTEA_P(0)));
}

Datum
adaptive_get_error(PG_FUNCTION_ARGS)
{

    /* return the error rate of the counter */
    PG_RETURN_FLOAT4(((AdaptiveCounter)PG_GETARG_BYTEA_P(0))->error);
    
}

Datum
adaptive_get_item_size(PG_FUNCTION_ARGS)
{
    /* return the error rate of the counter */
    PG_RETURN_INT32(((AdaptiveCounter)PG_GETARG_BYTEA_P(0))->itemSize);
}

Datum
adaptive_reset(PG_FUNCTION_ARGS)
{
	ac_reset(((AdaptiveCounter)PG_GETARG_BYTEA_P(0)));
	PG_RETURN_VOID();
}

Datum
adaptive_merge(PG_FUNCTION_ARGS)
{
	
	if (PG_ARGISNULL(0) && PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	} else if (PG_ARGISNULL(0)) {
		PG_RETURN_BYTEA_P(ac_create_copy((AdaptiveCounter)PG_GETARG_BYTEA_P(1)));
	} else if (PG_ARGISNULL(1)) {
        PG_RETURN_BYTEA_P(ac_create_copy((AdaptiveCounter)PG_GETARG_BYTEA_P(0)));
	}
	
	PG_RETURN_BYTEA_P(
        ac_merge(
            (AdaptiveCounter)PG_GETARG_BYTEA_P(0),
            (AdaptiveCounter)PG_GETARG_BYTEA_P(1)
        )
    );
}


/*
 *		byteain			- converts from printable representation of byte array
 *
 *		Non-printable characters must be passed as '\nnn' (octal) and are
 *		converted to internal form.  '\' must be passed as '\\'.
 *		ereport(ERROR, ...) if bad form.
 *
 *		BUGS:
 *				The input is scanned twice.
 *				The error checking of input is minimal.
 */
Datum
adaptive_in(PG_FUNCTION_ARGS)
{
	char	   *inputText = PG_GETARG_CSTRING(0);
	char	   *tp;
	char	   *rp;
	int			bc;
	bytea	   *result;

	/* Recognize hex input */
	if (inputText[0] == '\\' && inputText[1] == 'x')
	{
		size_t		len = strlen(inputText);

		bc = (len - 2) / 2 + VARHDRSZ;	/* maximum possible length */
		result = palloc(bc);
		bc = hex_decode(inputText + 2, len - 2, VARDATA(result));
		SET_VARSIZE(result, bc + VARHDRSZ);		/* actual length */

		PG_RETURN_BYTEA_P(result);
	}

	/* Else, it's the traditional escaped style */
	for (bc = 0, tp = inputText; *tp != '\0'; bc++)
	{
		if (tp[0] != '\\')
			tp++;
		else if ((tp[0] == '\\') &&
				 (tp[1] >= '0' && tp[1] <= '3') &&
				 (tp[2] >= '0' && tp[2] <= '7') &&
				 (tp[3] >= '0' && tp[3] <= '7'))
			tp += 4;
		else if ((tp[0] == '\\') &&
				 (tp[1] == '\\'))
			tp += 2;
		else
		{
			/*
			 * one backslash, not followed by another or ### valid octal
			 */
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input syntax for type bytea")));
		}
	}

	bc += VARHDRSZ;

	result = (bytea *) palloc(bc);
	SET_VARSIZE(result, bc);

	tp = inputText;
	rp = VARDATA(result);
	while (*tp != '\0')
	{
		if (tp[0] != '\\')
			*rp++ = *tp++;
		else if ((tp[0] == '\\') &&
				 (tp[1] >= '0' && tp[1] <= '3') &&
				 (tp[2] >= '0' && tp[2] <= '7') &&
				 (tp[3] >= '0' && tp[3] <= '7'))
		{
			bc = VAL(tp[1]);
			bc <<= 3;
			bc += VAL(tp[2]);
			bc <<= 3;
			*rp++ = bc + VAL(tp[3]);

			tp += 4;
		}
		else if ((tp[0] == '\\') &&
				 (tp[1] == '\\'))
		{
			*rp++ = '\\';
			tp += 2;
		}
		else
		{
			/*
			 * We should never get here. The first pass should not allow it.
			 */
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input syntax for type bytea")));
		}
	}

	PG_RETURN_BYTEA_P(result);
}

/*
 *		byteaout		- converts to printable representation of byte array
 *
 *		In the traditional escaped format, non-printable characters are
 *		printed as '\nnn' (octal) and '\' as '\\'.
 */
Datum
adaptive_out(PG_FUNCTION_ARGS)
{
	bytea	   *vlena = PG_GETARG_BYTEA_PP(0);
	char	   *result;
	char	   *rp;

	if (bytea_output == BYTEA_OUTPUT_HEX)
	{
		/* Print hex format */
		rp = result = palloc(VARSIZE_ANY_EXHDR(vlena) * 2 + 2 + 1);
		*rp++ = '\\';
		*rp++ = 'x';
		rp += hex_encode(VARDATA_ANY(vlena), VARSIZE_ANY_EXHDR(vlena), rp);
	}
	else if (bytea_output == BYTEA_OUTPUT_ESCAPE)
	{
		/* Print traditional escaped format */
		char	   *vp;
		int			len;
		int			i;

		len = 1;				/* empty string has 1 char */
		vp = VARDATA_ANY(vlena);
		for (i = VARSIZE_ANY_EXHDR(vlena); i != 0; i--, vp++)
		{
			if (*vp == '\\')
				len += 2;
			else if ((unsigned char) *vp < 0x20 || (unsigned char) *vp > 0x7e)
				len += 4;
			else
				len++;
		}
		rp = result = (char *) palloc(len);
		vp = VARDATA_ANY(vlena);
		for (i = VARSIZE_ANY_EXHDR(vlena); i != 0; i--, vp++)
		{
			if (*vp == '\\')
			{
				*rp++ = '\\';
				*rp++ = '\\';
			}
			else if ((unsigned char) *vp < 0x20 || (unsigned char) *vp > 0x7e)
			{
				int			val;	/* holds unprintable chars */

				val = *vp;
				rp[0] = '\\';
				rp[3] = DIG(val & 07);
				val >>= 3;
				rp[2] = DIG(val & 07);
				val >>= 3;
				rp[1] = DIG(val & 03);
				rp += 4;
			}
			else
				*rp++ = *vp;
		}
	}
	else
	{
		elog(ERROR, "unrecognized bytea_output setting: %d",
			 bytea_output);
		rp = result = NULL;		/* keep compiler quiet */
	}
	*rp = '\0';
	PG_RETURN_CSTRING(result);
}

/*
 *		bytearecv			- converts external binary format to bytea
 */
Datum
adaptive_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	bytea	   *result;
	int			nbytes;

	nbytes = buf->len - buf->cursor;
	result = (bytea *) palloc(nbytes + VARHDRSZ);
	SET_VARSIZE(result, nbytes + VARHDRSZ);
	pq_copymsgbytes(buf, VARDATA(result), nbytes);
	PG_RETURN_BYTEA_P(result);
}

/*
 *		byteasend			- converts bytea to binary format
 *
 * This is a special case: just copy the input...
 */
Datum
adaptive_send(PG_FUNCTION_ARGS)
{
	bytea	   *vlena = PG_GETARG_BYTEA_P_COPY(0);

	PG_RETURN_BYTEA_P(vlena);
}