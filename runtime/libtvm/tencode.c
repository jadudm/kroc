/*
tvm - tencode.c
The Transterpreter - a portable virtual machine for Transputer bytecode
Copyright (C) 2008 Carl G. Ritson

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "tvm.h"
#include "tvm_tbc.h"

WORD tenc_decode_int (BYTE *src)
{
	WORD value = *((WORD *) src);
	#if defined (TVM_BIG_ENDIAN)
	return value;
	#elif TVM_WORD_LENGTH == 4
	return SwapFourBytes (value);
	#elif TVM_WORD_LENGTH == 2
	return SwapTwoBytes (value);
	#else
	#error "Unknown TVM_WORD_LENGTH"
	#endif
}

int tenc_decode_element (BYTE *src, int *length, tenc_element_t *element)
{
	if (*length < (sizeof (WORD) + 4)) {
		return -1;
	}

	memcpy (element->id, src, 4);
	element->id[4] 	= '\0';
	element->length = tenc_decode_int (src + 4);
	element->next	= src + (sizeof (WORD) + 4);
	*length		-= (sizeof (WORD) + 4);

	if (element->id[3] == 'I' || element->id[3] == 'U') {
		element->data.u_int	= element->length;
		element->length		= 0;
		return 0;
	} else if (*length < element->length) {
		return -1;
	} else {
		const int align_mask = (sizeof (WORD) - 1);
		int bytes = element->length;

		if (bytes & align_mask) {
			bytes += (sizeof (WORD)) - (bytes & align_mask);
		}
		
		element->data.bytes 	= element->next;
		element->next		+= bytes;
		*length			-= bytes;

		return 0;
	}
}

static int ids_match (const char *a, const char *b)
{
	if (a[0] != b[0]) return 0;
	if (a[1] != b[1]) return 0;
	if (a[2] != b[2]) return 0;
	if (a[3] != b[3]) return 0;
	return 1;
}

int tenc_walk_to_element (BYTE *data, int *length, const char *id, tenc_element_t *element)
{
	while (*length > 0) {
		int ret = tenc_decode_element (data, length, element);

		if (ret < 0) {
			return ret;
		}

		if (ids_match (element->id, id)) {
			return 0;
		}
		
		data = element->next;
	}

	return -1;
}

static int load_uint (BYTE **data, int *length, const char *id, UWORD *dst)
{
	tenc_element_t element;
	int ret;

	if ((ret = tenc_walk_to_element (*data, length, id, &element)) < 0)
		return ret;
	
	*dst 	= element.data.u_int;
	*data	= element.next;

	return 0;
}

static int load_int (BYTE **data, int *length, const char *id, WORD *dst)
{
	return load_uint (data, length, id, (UWORD *) dst);
}

static int load_str (BYTE **data, int *length, const char *id, char **dst)
{
	tenc_element_t element;
	int ret;

	if ((ret = tenc_walk_to_element (*data, length, id, &element)) < 0)
		return ret;
	
	/* Make sure the string has room for a terminator */
	if (element.length < 1)
		return -1;

	*dst 	= element.data.str;
	*data	= element.next;

	/* Make sure the string is terminated */
	(*dst)[element.length - 1] = '\0';

	return 0;
}

static tenc_str_t *decode_strs (BYTE *data, int length, const char *id)
{
	tenc_str_t	*head	= NULL;
	tenc_str_t	*tail	= NULL;

	while (length > 0) {
		tenc_str_t *curr = (tenc_str_t *) data;
		char *str;

		if (load_str (&data, &length, id, &str) < 0)
			return head;
		
		curr->next	= NULL;
		curr->str	= str;

		if (tail == NULL) {
			head = tail = curr;
		} else {
			tail->next	= curr;
			tail		= curr;
		}
	}

	return head;
}

static tbc_tlp_t *decode_tlp (BYTE *head, const tenc_element_t *tlp_element)
{
	tbc_tlp_t *tlp 	= (tbc_tlp_t *) head;
	BYTE *data	= tlp_element->data.bytes;
	int length 	= tlp_element->length;

	if (load_str (&data, &length, "fmtS", &(tlp->fmt)) < 0)
		return NULL;
	
	if (load_str (&data, &length, "symS", &(tlp->symbol)) < 0)
		return NULL;
	
	return tlp;
}

static tbc_ffi_t *decode_ffi (BYTE *head, const tenc_element_t *ffi_element)
{
	tenc_element_t 	element;
	tenc_str_t	*sym;
	tbc_ffi_t	*ffi	= (tbc_ffi_t *) head;
	BYTE 		*data	= ffi_element->data.bytes;
	int 		length	= ffi_element->length;

	if (tenc_walk_to_element (data, &length, "libL", &element) < 0)
		return NULL;
	
	ffi->libraries	= decode_strs (element.data.bytes, element.length, "libS");
	data		= element.next;
	
	if (tenc_walk_to_element (data, &length, "symL", &element) < 0)
		return NULL;

	ffi->symbols	= decode_strs (element.data.bytes, element.length, "symS");
	data		= element.next;

	if (tenc_walk_to_element (data, &length, "mapL", &element) < 0)
		return NULL;
	
	ffi->n_symbols	= 0;
	ffi->map 	= (tbc_ffi_entry_t *) (element.data.bytes - (4 + sizeof (WORD)));
	length		= element.length;
	data		= element.data.bytes;
	sym		= ffi->symbols;
	while ((sym != NULL) && (length > 0)) {
		WORD lib_idx;

		if (load_int (&data, &length, "idxI", &lib_idx) < 0)
			return NULL;

		ffi->map[ffi->n_symbols].symbol = sym->str;

		if (lib_idx < 0) {
			ffi->map[ffi->n_symbols].library = NULL;
		} else {
			tenc_str_t *lib = ffi->libraries;
			
			while (lib != NULL && lib_idx > 0) {
				lib_idx--;
				lib = lib->next;
			}

			if (lib == NULL)
				return NULL;

			ffi->map[ffi->n_symbols].library = lib->str;
		}

		ffi->n_symbols++;
		sym = sym->next;
	}
	
	if (sym != NULL)
		return NULL;

	return ffi;
}

static tbc_dbg_t *decode_debug (BYTE *head, const tenc_element_t *dbg_element)
{
	tenc_element_t 	element;
	tbc_dbg_t	*dbg	= (tbc_dbg_t *) head;
	BYTE 		*data	= dbg_element->data.bytes;
	int 		length	= dbg_element->length;

	if (tenc_walk_to_element (data, &length, "fn L", &element) < 0)
		return NULL;

	dbg->files 	= decode_strs (element.data.bytes, element.length, "fn S");
	data		= element.next;
	
	if (tenc_walk_to_element (data, &length, "lndB", &element) < 0)
		return NULL;

	dbg->lnd 	= (tbc_lnd_t *) element.next;
	dbg->n_lnd	= element.length / ((sizeof (int)) * 3);
	
	data		= element.next;
	length		= element.length / (sizeof (int));
	while (length > 0) {
		*((int *) data) = tenc_decode_int (data);
		data += sizeof (int);
		length--;
	}

	return dbg;
}

static tbc_sym_t *decode_symbols (const tenc_element_t *stb_element)
{
	tenc_element_t	element;
	tbc_sym_t	*head	= NULL;
	tbc_sym_t	*sym	= NULL;
	BYTE 		*data	= stb_element->data.bytes;
	int 		length	= stb_element->length;

	while (length > 0) {
		BYTE 		*s_data;
		int		s_length;

		if (tenc_walk_to_element (data, &length, "symL", &element) < 0)
			return NULL;

		if (head == NULL) {
			head 		= (tbc_sym_t *) data;
			sym		= head;
		} else {
			sym->next	= (tbc_sym_t *) data;
			sym		= sym->next;
		}

		s_data		= element.data.bytes;
		s_length	= element.length;
		data		= element.next;

		if (load_uint (&s_data, &s_length, "offU", &(sym->offset)) < 0)
			return NULL;
		if (load_str (&s_data, &s_length, "symS", &(sym->name)) < 0)
			return NULL;

		sym->definition	= NULL;
		sym->ws		= 0;
		sym->vs		= 0;

		if (s_length > 0) {
			if (load_str (&s_data, &s_length, "defS", &(sym->definition)) < 0)
				continue;
			if (load_uint (&s_data, &s_length, "ws U", &(sym->ws)) < 0)
				continue;
			if (load_uint (&s_data, &s_length, "vs U", &(sym->vs)) < 0)
				continue;

		}
	}

	return head;
}


int tbc_decode (BYTE *data, int length, tbc_t **ptr)
{
	tenc_element_t 	element;
	tbc_t 		*tbc	= (tbc_t *) data;
	int		ret;

	/* Decode the required elements */

	if ((ret = load_uint (&data, &length, "endU", &(tbc->endian))) < 0)
		return ret;
	if ((ret = load_uint (&data, &length, "ws U", &(tbc->ws))) < 0)
		return ret;
	if ((ret = load_uint (&data, &length, "vs U", &(tbc->vs))) < 0)
		return ret;

	if ((ret = tenc_walk_to_element (data, &length, "bc B", &element)) < 0)
		return ret;
	
	tbc->bytecode_len	= element.length;
	tbc->bytecode		= element.data.bytes;
	data			= element.next;

	/* Decode optional elements */
	tbc->tlp	= NULL;
	tbc->symbols	= NULL;
	tbc->ffi	= NULL;
	tbc->debug	= NULL;

	/* Copy pointer */
	*ptr 		= tbc;

	while (length > 0) {
		if (tenc_decode_element (data, &length, &element) < 0)
			return 0; /* ignore errors */

		if (ids_match (element.id, "tlpL")) {
			tbc->tlp = decode_tlp (data, &element); 
		} else if (ids_match (element.id, "ffiL")) {
			tbc->ffi = decode_ffi (data, &element);
		} else if (ids_match (element.id, "stbL")) {
			tbc->symbols = decode_symbols (&element);
		} else if (ids_match (element.id, "dbgL")) {
			tbc->debug = decode_debug (data, &element);
		}

		data = element.next;
	}

	return 0;
}

