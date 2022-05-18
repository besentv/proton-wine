/* WinRT Windows.Media.Speech implementation
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include "initguid.h"
#include "private.h"

#include "winternl.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(speech);

unixlib_handle_t vosk_handle = 0;

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, void *reserved)
{
    NTSTATUS status;
    TRACE("dll %p, reason %lu, reserved %p.\n", dll, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            if ((status = NtQueryVirtualMemory(GetCurrentProcess(), dll, MemoryWineUnixFuncs,
                                              &vosk_handle, sizeof(vosk_handle), NULL)))
                {
                    WARN("Querying for Unixlib failed with status %lu.\n", status);
                    return FALSE;
                }
            break;
    }
    return TRUE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    const WCHAR *buffer = WindowsGetStringRawBuffer(classid, NULL);

    TRACE("classid %s, factory %p.\n", debugstr_w(buffer), factory);

    *factory = NULL;

    if (!wcscmp(buffer, L"Windows.Media.SpeechRecognition.SpeechRecognizer"))
        IActivationFactory_AddRef((*factory = recognizer_factory));
    if (!wcscmp(buffer, L"Windows.Media.SpeechRecognition.SpeechRecognitionListConstraint"))
        IActivationFactory_AddRef((*factory = listconstraint_factory));
    if (!wcscmp(buffer, L"Windows.Media.SpeechSynthesis.SpeechSynthesizer"))
        IActivationFactory_AddRef((*factory = synthesizer_factory));

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
