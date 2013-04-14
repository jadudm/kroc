/*
 *	ocuda_ctypes.h -- occam-pi/CUDA C types.
 *	Copyright (C) 2013 Fred Barnes <frmb@kent.ac.uk>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __OCUDA_CTYPES_H
#define __OCUDA_CTYPES_H

/* defines a CUDA device (basic) */

typedef struct TAG_ocuda_devinfo {
	int dnum;
	char dname[64];
	int dname_len;
	int cversion;
} ocuda_devinfo_t;

/* available to other CUDA code: */

int __get_last_cuda_error (const char *, const char *, const int);
#define get_last_cuda_error(MSG) __get_last_cuda_error (MSG, __FILE__, __LINE__)


#endif	/* !__OCUDA_CTYPE_H */

