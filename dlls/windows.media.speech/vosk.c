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

#define MAKE_FUNCPTR( f ) static typeof(f) * p##f;
MAKE_FUNCPTR( vosk_model_new )
MAKE_FUNCPTR( vosk_recognizer_new )
MAKE_FUNCPTR( vosk_model_free )
MAKE_FUNCPTR( vosk_recognizer_free )
#undef MAKE_FUNCPTR

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

#define LOAD_FUNCPTR( f ) \
    do if((p##f = dlsym( libvosk_handle, #f )) == NULL) \
    { \
        ERR( "Failed to load symbol %s\n", #f ); \
        goto error; \
    } while(0)
    LOAD_FUNCPTR( vosk_model_new );
    LOAD_FUNCPTR( vosk_recognizer_new );
    LOAD_FUNCPTR( vosk_model_free );
    LOAD_FUNCPTR( vosk_recognizer_free );
#undef LOAD_FUNCPTR
    return STATUS_SUCCESS;
error:
    dlclose( libvosk_handle );
    libvosk_handle = NULL;
    return STATUS_ENTRYPOINT_NOT_FOUND;
#else /* SONAME_LIBVOSK */
    return STATUS_NOT_SUPPORTED;
#endif /* SONAME_LIBVOSK */
}

static inline vosk_instance to_vosk_instance( VoskRecognizer *ptr )
{
    return (vosk_instance)(UINT_PTR)ptr;
}

static inline VoskRecognizer *from_vosk_instance( vosk_instance instance )
{
    return (VoskRecognizer *)(UINT_PTR)instance;
}

static NTSTATUS create( void *args )
{
    struct vosk_create_params *params = args;
    VoskRecognizer *recognizer = NULL;
    VoskModel *model = NULL;

    TRACE( "args %p.\n", args );

    if (!pvosk_model_new || !pvosk_recognizer_free || !pvosk_recognizer_new || !pvosk_model_free)
        return STATUS_UNSUCCESSFUL;

    model = pvosk_model_new( params->model_path );
    if (model == NULL) goto error;

    recognizer = pvosk_recognizer_new( model, params->sample_rate );
    if (model == NULL) goto error;

    /* The model is kept alive inside the recognizer, so we can safely free our ref here. */
    pvosk_model_free( model );

    *params->instance = to_vosk_instance( recognizer );
    return STATUS_SUCCESS;

error:
    *params->instance = to_vosk_instance( NULL );
    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS release( void *args )
{
    struct vosk_release_params *params = args;

    TRACE( "args %p.\n", args );

    if (!params->instance || !pvosk_recognizer_free)
        return STATUS_UNSUCCESSFUL;

    pvosk_recognizer_free( from_vosk_instance( params->instance ) );

    return STATUS_SUCCESS;
}

#else /* HAVE_VOSK_API_H */

static NTSTATUS process_attach( void *args ) { return STATUS_NOT_SUPPORTED; }
static NTSTATUS create( void *args ) { return STATUS_NOT_SUPPORTED; }
static NTSTATUS release( void *args ) { return STATUS_NOT_SUPPORTED; }

#endif /* HAVE_VOSK_API_H */

unixlib_entry_t __wine_unix_call_funcs[] =
{
    process_attach,
    create,
    release
};

unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    process_attach,
    create,
    release
};
