/*
 * Vosk interface (Unixlib) for Windows.Media.Speech
 *
 * Copyright 2022 Bernhard Kölbl for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <dlfcn.h>

#ifdef HAVE_VOSK_API_H
#include <vosk_api.h>
#endif /* HAVE_VOSK_API_H */

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winerror.h"
#include "winternl.h"

#include "wine/debug.h"

#include "unixlib.h"

#ifdef HAVE_VOSK_API_H

WINE_DEFAULT_DEBUG_CHANNEL( vosk );

#ifdef SONAME_LIBVOSK
static void *libvosk_handle;
#endif /* SONAME_LIBVOSK */

static NTSTATUS process_attach( void *args )
{
    TRACE( "args %p.\n", args );

#ifdef SONAME_LIBVOSK
    if (!(libvosk_handle = dlopen( SONAME_LIBVOSK, RTLD_NOW )))
    {
        WARN( "Failed to load library %s, reason %s.\n", SONAME_LIBVOSK, dlerror() );
        return STATUS_DLL_NOT_FOUND;
    }

    return STATUS_SUCCESS;
#else /* SONAME_LIBVOSK */
    return STATUS_NOT_SUPPORTED;
#endif /* SONAME_LIBVOSK */
}

#else /* HAVE_VOSK_API_H */

static NTSTATUS process_attach( void *args ) { return STATUS_NOT_SUPPORTED; }

#endif /* HAVE_VOSK_API_H */

unixlib_entry_t __wine_unix_call_funcs[] =
{
    process_attach
};

unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    process_attach
};
