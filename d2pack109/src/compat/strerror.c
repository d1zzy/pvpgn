/*
 * Copyright (C) 1999  Ross Combs (rocombs@cs.nmsu.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "common/setup_before.h"
#ifndef HAVE_STRERROR

#include <errno.h>
#include "strerror.h"
#include "common/setup_after.h"


extern char const * strerror(int errornum)
{
    if (errornum==0)
	return "No error";
#ifdef EPERM
    if (errornum==EPERM)
	return "Operation not permitted";
#endif
#ifdef ENOENT
    if (errornum==ENOENT)
	return "No such file or directory";
#endif
#ifdef ESRCH
    if (errornum==ESRCH)
	return "No such process";
#endif
#ifdef EINTR
    if (errornum==EINTR)
	return "Interrupted system call";
#endif
#ifdef EIO
    if (errornum==EIO)
	return "I/O error";
#endif
#ifdef ENXIO
    if (errornum==EIO)
	return "No such device or address";
#endif
#ifdef EBADF
    if (errornum==EBADF)
	return "Bad file number";
#endif
#ifdef EAGAIN
    if (errornum==EAGAIN)
	return "Try again";
#endif
#ifdef ENOMEM
    if (errornum==ENOMEM)
	return "Out of memory";
#endif
#ifdef EACCES
    if (errornum==EACCES)
	return "Permission denied";
#endif
#ifdef EFAULT
    if (errornum==EFAULT)
	return "Bad address";
#endif
#ifdef EBUSY
    if (errornum==EBUSY)
	return "Device or resource busy";
#endif
#ifdef EEXIST
    if (errornum==EEXIST)
	return "File exists";
#endif
#ifdef EXDEV
    if (errornum==EXDEV)
	return "Cross-device link";
#endif
#ifdef EDEADLK
    if (errornum==EXDEV)
	return "Resource deadlock would occur";
#endif
#ifdef EDEADLOCK
    if (errornum==EDEADLOCK)
	return "Resource deadlock would occur";
#endif
#ifdef ENODEV
    if (errornum==ENODEV)
	return "No such device";
#endif
#ifdef ENOTDIR
    if (errornum==ENOTDIR)
	return "Not a directory";
#endif
#ifdef EISDIR
    if (errornum==EISDIR)
	return "Is a directory";
#endif
#ifdef EINVAL
    if (errornum==EINVAL)
	return "Invalid argument";
#endif
#ifdef ENFILE
    if (errornum==ENFILE)
	return "Too many open files in system";
#endif
#ifdef EMFILE
    if (errornum==EMFILE)
	return "Too many open files";
#endif
#ifdef ENOTTY
    if (errornum==ENOTTY)
	return "Not a typewriter";
#endif
#ifdef ETXTBSY
    if (errornum==ETXTBSY)
	return "Text file busy";
#endif
#ifdef EFBIG
    if (errornum==EFBIG)
	return "File too large";
#endif
#ifdef ENOSPC
    if (errornum==ENOSPC)
	return "No space left on device";
#endif
#ifdef ESPIPE
    if (errornum==ESPIPE)
	return "Illegal seek";
#endif
#ifdef EROFS
    if (errornum==EROFS)
	return "Read-only file system";
#endif
#ifdef EMLINK
    if (errornum==EMLINK)
	return "Too many links";
#endif
#ifdef EPIPE
    if (errornum==EPIPE)
	return "Broken pipe";
#endif
#ifdef EDOM
    if (errornum==EDOM)
	return "Math argument out of domain of func";
#endif
#ifdef ERANGE
    if (errornum==ERANGE)
	return "Math result not representable";
#endif
#ifdef ENAMETOOLONG
    if (errornum==ENAMETOOLONG)
	return "File name too long";
#endif
#ifdef ENOLCK
    if (errornum==ENOLCK)
	return "No record locks available";
#endif
#ifdef ENOSYS
    if (errornum==ENOSYS)
	return "Function not implemented";
#endif
#ifdef ENOTEMPTY
    if (errornum==ENOTEMPTY)
	return "Directory not empty";
#endif
#ifdef ELOOP
    if (errornum==ELOOP)
	return "Too many symbolic links encountered";
#endif
#ifdef EHOSTDOWN
    if (errornum==EHOSTDOWN)
	return "Host is down";
#endif
#ifdef EHOSTUNREACH
    if (errornum==EHOSTUNREACH)
	return "No route to host";
#endif
#ifdef EALREADY
    if (errornum==EALREADY)
	return "Operation already in progress";
#endif
#ifdef EINPROGRESS
    if (errornum==EINPROGRESS)
	return "Operation now in progress";
#endif
#ifdef ESTALE
    if (errornum==ESTALE)
	return "Stale NFS filehandle";
#endif
#ifdef EDQUOT
    if (errornum==EDQUOT)
	return "Quota exceeded";
#endif
#ifdef EWOULDBLOCK
    if (errornum==EWOULDBLOCK)
	return "Operation would block";
#endif
#ifdef ECOMM
    if (errornum==ECOMM)
	return "Communication error on send";
#endif
#ifdef EPROTO
    if (errornum==EPROTO)
	return "Protocol error";
#endif
#ifdef EPROTONOSUPPORT
    if (errornum==EPROTONOSUPPORT)
	return "Protocol not supported";
#endif
#ifdef ESOCKTNOSUPPORT
    if (errornum==ESOCKTNOSUPPORT)
	return "Socket type not supported";
#endif
#ifdef ESOCKTNOSUPPORT
    if (errornum==EOPNOTSUPP)
	return "Operation not supported";
#endif
#ifdef EPFNOSUPPORT
    if (errornum==EPFNOSUPPORT)
	return "Protocol family not supported";
#endif
#ifdef EAFNOSUPPORT
    if (errornum==EAFNOSUPPORT)
	return "Address family not supported by protocol family";
#endif
#ifdef EADDRINUSE
    if (errornum==EADDRINUSE)
	return "Address already in use";
#endif
#ifdef EADDRNOTAVAIL
    if (errornum==EADDRNOTAVAIL)
	return "Cannot assign requested address";
#endif
#ifdef ENETDOWN
    if (errornum==ENETDOWN)
	return "Network is down";
#endif
#ifdef ENETUNREACH
    if (errornum==ENETUNREACH)
	return "Network is unreachable";
#endif
#ifdef ENETRESET
    if (errornum==ENETRESET)
	return "Network dropped connection on reset";
#endif
#ifdef ECONNABORTED
    if (errornum==ECONNABORTED)
	return "Software caused connection abort";
#endif
#ifdef ECONNRESET
    if (errornum==ECONNRESET)
	return " Connection reset by peer";
#endif
#ifdef ENOBUFS
    if (errornum==ENOBUFS)
	return "No buffer space available";
#endif
#ifdef EISCONN
    if (errornum==EISCONN)
	return "Socket is already connected";
#endif
#ifdef ENOTCONN
    if (errornum==ENOTCONN)
	return "Socket is not connected";
#endif
#ifdef ESHUTDOWN
    if (errornum==ESHUTDOWN)
	return " Cannot send after socket shutdown";
#endif
#ifdef ETIMEDOUT
    if (errornum==ETIMEDOUT)
	return "Connection timed out";
#endif
#ifdef ECONNREFUSED
    if (errornum==ECONNREFUSED)
	return "Connection refused";
#endif
    return "Unknown error";
}

#else
typedef int filenotempty; /* make ISO standard happy */
#endif
