/****************************************************************************
 *               Low-level manipulation of ordinary R vectors               *
 ****************************************************************************/
#include "S4Vectors.h"


/*
 * memcmp()-based comparison of 2 vectors of the same type.
 * NOTE: Doesn't support STRSXP and VECSXP.
 */
int _vector_memcmp(SEXP x1, int x1_offset, SEXP x2, int x2_offset, int nelt)
{
	const void *s1 = NULL, *s2 = NULL; /* gcc -Wall */
	size_t eltsize = 0; /* gcc -Wall */

	if (x1_offset < 0 || x1_offset + nelt > LENGTH(x1)
	 || x2_offset < 0 || x2_offset + nelt > LENGTH(x2))
		error("S4Vectors internal error in _vector_memcmp(): "
		      "elements to compare are out of vector bounds");
	switch (TYPEOF(x1)) {
	case RAWSXP:
		s1 = (const void *) (RAW(x1) + x1_offset);
		s2 = (const void *) (RAW(x2) + x2_offset);
		eltsize = sizeof(Rbyte);
		break;
	case LGLSXP:
	case INTSXP:
		s1 = (const void *) (INTEGER(x1) + x1_offset);
		s2 = (const void *) (INTEGER(x2) + x2_offset);
		eltsize = sizeof(int);
		break;
	case REALSXP:
		s1 = (const void *) (REAL(x1) + x1_offset);
		s2 = (const void *) (REAL(x2) + x2_offset);
		eltsize = sizeof(double);
		break;
	case CPLXSXP:
		s1 = (const void *) (COMPLEX(x1) + x1_offset);
		s2 = (const void *) (COMPLEX(x2) + x2_offset);
		eltsize = sizeof(Rcomplex);
		break;
	default:
		error("S4Vectors internal error in _vector_memcmp(): "
		      "%s type not supported", CHAR(type2str(TYPEOF(x1))));
	}
	return s1 == s2 ? 0 : memcmp(s1, s2, nelt * eltsize);
}

/*
 * memcpy()-based copy of data from a vector to a vector of the same type.
 */
void _vector_memcpy(SEXP out, int out_offset, SEXP in, int in_offset, int nelt)
{
	void *dest;
	const void *src;
	size_t eltsize;
	int i;
	SEXP in_elt; // out_elt;

	if (out_offset < 0 || out_offset + nelt > LENGTH(out)
	 || in_offset < 0 || in_offset + nelt > LENGTH(in))
		error("subscripts out of bounds");
	switch (TYPEOF(out)) {
	    case RAWSXP:
		dest = (void *) (RAW(out) + out_offset);
		src = (const void *) (RAW(in) + in_offset);
		eltsize = sizeof(Rbyte);
		break;
	    case LGLSXP:
		dest = (void *) (LOGICAL(out) + out_offset);
		src = (const void *) (LOGICAL(in) + in_offset);
		eltsize = sizeof(int);
		break;
	    case INTSXP:
		dest = (void *) (INTEGER(out) + out_offset);
		src = (const void *) (INTEGER(in) + in_offset);
		eltsize = sizeof(int);
		break;
	    case REALSXP:
		dest = (void *) (REAL(out) + out_offset);
		src = (const void *) (REAL(in) + in_offset);
		eltsize = sizeof(double);
		break;
	    case CPLXSXP:
		dest = (void *) (COMPLEX(out) + out_offset);
		src = (const void *) (COMPLEX(in) + in_offset);
		eltsize = sizeof(Rcomplex);
		break;
	    case STRSXP:
		for (i = 0; i < nelt; i++) {
			in_elt = STRING_ELT(in, in_offset + i);
			SET_STRING_ELT(out, out_offset + i, in_elt);
			//PROTECT(out_elt = duplicate(in_elt));
			//SET_STRING_ELT(out, out_offset + i, out_elt);
			//UNPROTECT(1);
		}
		return;
	    case VECSXP:
		for (i = 0; i < nelt; i++) {
			in_elt = VECTOR_ELT(in, in_offset + i);
			SET_VECTOR_ELT(out, out_offset + i, in_elt);
			//PROTECT(out_elt = duplicate(in_elt));
			//SET_VECTOR_ELT(out, out_offset + i, out_elt);
			//UNPROTECT(1);
		}
		return;
	    default:
		error("S4Vectors internal error in _vector_memcpy(): "
		      "%s type not supported", CHAR(type2str(TYPEOF(out))));
		return; // gcc -Wall
	}
	memcpy(dest, src, nelt * eltsize);
	return;
}

static int get_NROW(SEXP x)
{
	SEXP x_dim, x_rownames;

	if (x == R_NilValue)
		return 0;
	if (!IS_VECTOR(x))
		error("get_NROW() defined only on a vector (or NULL)");
	/* A data.frame doesn't have a "dim" attribute but the dimensions can
	   be inferred from the "names" and "row.names" attributes. */
	x_rownames = getAttrib(x, R_RowNamesSymbol);
	if (x_rownames != R_NilValue)
		return LENGTH(x_rownames);
	x_dim = GET_DIM(x);
	if (x_dim == R_NilValue || LENGTH(x_dim) == 0)
		return LENGTH(x);
	return INTEGER(x_dim)[0];
}

/*
 * --- .Call ENTRY POINT ---
 * A C implementation of 'sapply(x, NROW)' that works only on a list of
 * vectors (or NULLs).
 */
SEXP sapply_NROW(SEXP x)
{
	SEXP ans, x_elt;
	int x_len, i, *ans_elt;

	x_len = LENGTH(x);
	PROTECT(ans = NEW_INTEGER(x_len));
	for (i = 0, ans_elt = INTEGER(ans); i < x_len; i++, ans_elt++) {
		x_elt = VECTOR_ELT(x, i);
		if (x_elt != R_NilValue && !IS_VECTOR(x_elt)) {
			UNPROTECT(1);
			error("element %d not a vector (or NULL)", i + 1);
		}
		*ans_elt = get_NROW(x_elt);
	}
	UNPROTECT(1);
	return ans;
}
