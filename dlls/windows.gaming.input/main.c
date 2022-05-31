/* WinRT Windows.Gaming.Input implementation
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

#include <stdarg.h>
#include <stddef.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "initguid.h"
#include "private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING class_str, IActivationFactory **factory )
{
    const WCHAR *buffer = WindowsGetStringRawBuffer( class_str, NULL );

    TRACE( "class %s, factory %p.\n", debugstr_w(buffer), factory );

    *factory = NULL;

    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_RawGameController ))
        IActivationFactory_AddRef( (*factory = controller_factory) );
    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_Gamepad ))
        IActivationFactory_AddRef( (*factory = gamepad_factory) );

    if (*factory) return S_OK;
    return REGDB_E_CLASSNOTREG;
}
