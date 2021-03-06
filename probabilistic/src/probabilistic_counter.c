#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>

#include "postgres.h"
#include "fmgr.h"
#include "probabilistic.h"
#include "utils/builtins.h"
#include "utils/bytea.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define VAL(CH)         ((CH) - '0')
#define DIG(VAL)        ((VAL) + '0')

#define DEFAULT_NBYTES  4
#define DEFAULT_NSALTS  32
#define MAX_NBYTES      16
#define MAX_NSALTS      1024

PG_FUNCTION_INFO_V1(probabilistic_add_item_text);
PG_FUNCTION_INFO_V1(probabilistic_add_item_int);

PG_FUNCTION_INFO_V1(probabilistic_add_item_agg_text);
PG_FUNCTION_INFO_V1(probabilistic_add_item_agg_int);

PG_FUNCTION_INFO_V1(probabilistic_add_item_agg2_text);
PG_FUNCTION_INFO_V1(probabilistic_add_item_agg2_int);

PG_FUNCTION_INFO_V1(probabilistic_get_estimate);
PG_FUNCTION_INFO_V1(probabilistic_size);
PG_FUNCTION_INFO_V1(probabilistic_init);
PG_FUNCTION_INFO_V1(probabilistic_reset);
PG_FUNCTION_INFO_V1(probabilistic_in);
PG_FUNCTION_INFO_V1(probabilistic_out);
PG_FUNCTION_INFO_V1(probabilistic_rect);
PG_FUNCTION_INFO_V1(probabilistic_send);
PG_FUNCTION_INFO_V1(probabilistic_length);

Datum probabilistic_add_item_text(PG_FUNCTION_ARGS);
Datum probabilistic_add_item_int(PG_FUNCTION_ARGS);

Datum probabilistic_add_item_agg_text(PG_FUNCTION_ARGS);
Datum probabilistic_add_item_agg_int(PG_FUNCTION_ARGS);

Datum probabilistic_add_item_agg2_text(PG_FUNCTION_ARGS);
Datum probabilistic_add_item_agg2_int(PG_FUNCTION_ARGS);

Datum probabilistic_get_estimate(PG_FUNCTION_ARGS);
Datum probabilistic_size(PG_FUNCTION_ARGS);
Datum probabilistic_init(PG_FUNCTION_ARGS);
Datum probabilistic_reset(PG_FUNCTION_ARGS);
Datum probabilistic_in(PG_FUNCTION_ARGS);
Datum probabilistic_out(PG_FUNCTION_ARGS);
Datum probabilistic_recv(PG_FUNCTION_ARGS);
Datum probabilistic_send(PG_FUNCTION_ARGS);
Datum probabilistic_length(PG_FUNCTION_ARGS);

Datum
probabilistic_add_item_text(PG_FUNCTION_ARGS)
{

    ProbabilisticCounter pc;
    text * item;
    
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if ((! PG_ARGISNULL(0)) && (! PG_ARGISNULL(1))) {

      pc = (ProbabilisticCounter)PG_GETARG_BYTEA_P(0);
      
      /* get the new item */
      item = PG_GETARG_TEXT_P(1);
    
      /* in-place update works only if executed as aggregate */
      pc_add_element_text(pc, VARDATA(item), VARSIZE(item) - VARHDRSZ);
      
    } else if (PG_ARGISNULL(0)) {
        elog(ERROR, "probabilistic counter must not be NULL");
    }
    
    PG_RETURN_VOID();

}

Datum
probabilistic_add_item_int(PG_FUNCTION_ARGS)
{

    ProbabilisticCounter pc;
    int item;
    
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if ((! PG_ARGISNULL(0)) && (! PG_ARGISNULL(1))) {

        pc = (ProbabilisticCounter)PG_GETARG_BYTEA_P(0);
        
        /* get the new item */
        item = PG_GETARG_INT32(1);
        
        /* in-place update works only if executed as aggregate */
        pc_add_element_int(pc, item);
      
    } else if (PG_ARGISNULL(0)) {
        elog(ERROR, "probabilistic counter must not be NULL");
    }
    
    PG_RETURN_VOID();

}

Datum
probabilistic_add_item_agg_text(PG_FUNCTION_ARGS)
{
	
    ProbabilisticCounter pc;
    text * item;
    int  nbytes; /* number of bytes per salt */
    int  nsalts; /* number of salts */
  
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if (PG_ARGISNULL(0)) {
      nbytes = PG_GETARG_INT32(2);
      nsalts = PG_GETARG_INT32(3);
      
      /* nbytes and nsalts have to be positive */
      if ((nbytes < 1) || (nbytes > MAX_NBYTES)) {
          elog(ERROR, "number of bytes per bitmap has to be between 1 and %d", MAX_NBYTES);
      } else if (nsalts < 1) {
          elog(ERROR, "number salts has to be between 1 and %d", MAX_NSALTS);
      }
      
      pc = pc_create(nbytes, nsalts);
    } else {
      pc = (ProbabilisticCounter)PG_GETARG_BYTEA_P(0);
    }
      
    /* get the new item */
    item = PG_GETARG_TEXT_P(1);
    
    /* in-place update works only if executed as aggregate */
    pc_add_element_text(pc, VARDATA(item), VARSIZE(item) - VARHDRSZ);
    
    /* return the updated bytea */
    PG_RETURN_BYTEA_P(pc);
    
}

Datum
probabilistic_add_item_agg_int(PG_FUNCTION_ARGS)
{
    
    ProbabilisticCounter pc;
    int item;
    int  nbytes; /* number of bytes per salt */
    int  nsalts; /* number of salts */
  
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if (PG_ARGISNULL(0)) {
      nbytes = PG_GETARG_INT32(2);
      nsalts = PG_GETARG_INT32(3);
      
      /* nbytes and nsalts have to be positive */
      if ((nbytes < 1) || (nbytes > MAX_NBYTES)) {
          elog(ERROR, "number of bytes per bitmap has to be between 1 and %d", MAX_NBYTES);
      } else if (nsalts < 1) {
          elog(ERROR, "number salts has to be between 1 and %d", MAX_NSALTS);
      }
      
      pc = pc_create(nbytes, nsalts);
    } else {
      pc = (ProbabilisticCounter)PG_GETARG_BYTEA_P(0);
    }
      
    /* get the new item */
    item = PG_GETARG_INT32(1);
    
    /* in-place update works only if executed as aggregate */
    pc_add_element_int(pc, item);
    
    /* return the updated bytea */
    PG_RETURN_BYTEA_P(pc);
    
}

Datum
probabilistic_add_item_agg2_text(PG_FUNCTION_ARGS)
{
    
    ProbabilisticCounter pc;
    text * item;
  
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if (PG_ARGISNULL(0)) {
      pc = pc_create(DEFAULT_NBYTES, DEFAULT_NSALTS);
    } else {
      pc = (ProbabilisticCounter)PG_GETARG_BYTEA_P(0);
    }
      
    /* get the new item */
    item = PG_GETARG_TEXT_P(1);
    
    /* in-place update works only if executed as aggregate */
    pc_add_element_text(pc, VARDATA(item), VARSIZE(item) - VARHDRSZ);
    
    /* return the updated bytea */
    PG_RETURN_BYTEA_P(pc);
    
}

Datum
probabilistic_add_item_agg2_int(PG_FUNCTION_ARGS)
{
    
    ProbabilisticCounter pc;
    int item;
  
    /* is the counter created (if not, create it - error 1%, 10mil items) */
    if (PG_ARGISNULL(0)) {
      pc = pc_create(DEFAULT_NBYTES, DEFAULT_NSALTS);
    } else {
      pc = (ProbabilisticCounter)PG_GETARG_BYTEA_P(0);
    }
      
    /* get the new item */
    item = PG_GETARG_INT32(1);
    
    /* in-place update works only if executed as aggregate */
    pc_add_element_int(pc, item);
    
    /* return the updated bytea */
    PG_RETURN_BYTEA_P(pc);
    
}

Datum
probabilistic_get_estimate(PG_FUNCTION_ARGS)
{
  
    int estimate;
    ProbabilisticCounter pc = (ProbabilisticCounter)PG_GETARG_BYTEA_P(0);
    
    /* in-place update works only if executed as aggregate */
    estimate = pc_estimate(pc);
    
    /* return the updated bytea */
    PG_RETURN_FLOAT4(estimate);

}

Datum
probabilistic_init(PG_FUNCTION_ARGS)
{
      ProbabilisticCounter pc;
      int nbytes;
      int nsalts;
      
      nbytes = PG_GETARG_INT32(0);
      nsalts = PG_GETARG_INT32(1);
      
      /* nbytes and nsalts have to be positive */
      if ((nbytes < 1) || (nbytes > MAX_NBYTES)) {
          elog(ERROR, "number of bytes per bitmap has to be between 1 and %d", MAX_NBYTES);
      } else if (nsalts < 1) {
          elog(ERROR, "number salts has to be between 1 and %d", MAX_NSALTS);
      }
      
      pc = pc_create(nbytes, nsalts);
      
      PG_RETURN_BYTEA_P(pc);
}

Datum
probabilistic_size(PG_FUNCTION_ARGS)
{
    
    int nbytes, nsalts;
    
    nbytes = PG_GETARG_INT32(0);
    nsalts = PG_GETARG_INT32(1);
    
    PG_RETURN_INT32(pc_size(nbytes, nsalts));
    
}

Datum
probabilistic_length(PG_FUNCTION_ARGS)
{
    PG_RETURN_INT32(VARSIZE((ProbabilisticCounter)PG_GETARG_BYTEA_P(0)));
}

Datum
probabilistic_reset(PG_FUNCTION_ARGS)
{
	pc_reset(((ProbabilisticCounter)PG_GETARG_BYTEA_P(0)));
	PG_RETURN_VOID();
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
probabilistic_in(PG_FUNCTION_ARGS)
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
probabilistic_out(PG_FUNCTION_ARGS)
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
probabilistic_recv(PG_FUNCTION_ARGS)
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
probabilistic_send(PG_FUNCTION_ARGS)
{
	bytea	   *vlena = PG_GETARG_BYTEA_P_COPY(0);

	PG_RETURN_BYTEA_P(vlena);
}
