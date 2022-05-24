/*
 * Unix library interface for Windows.Media.Speech
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

#include "wine/unixlib.h"

typedef UINT64 vosk_instance;

struct vosk_create_params
{
    vosk_instance *instance;
    const char *model_path;
    float sample_rate;
};

struct vosk_release_params
{
    vosk_instance instance;
};

struct recognize_audio_params
{
    vosk_instance instance;
    const BYTE *samples;
    UINT32 samples_size;
    INT32 status;
};


enum vosk_funcs
{
    vosk_process_attach,
    vosk_create,
    vosk_release,
    vosk_recognize_audio
};

extern unixlib_handle_t vosk_handle;

#define VOSK_CALL(func, params) __wine_unix_call(vosk_handle, vosk_ ## func, params)
