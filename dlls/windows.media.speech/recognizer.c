/* WinRT Windows.Media.SpeechRecognition implementation
 *
 * Copyright 2022 Bernhard Kölbl
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

#include "private.h"
#include "initguid.h"

#include <wchar.h>

#include "audioclient.h"
#include "mmdeviceapi.h"

#include "wine/debug.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(speech);

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

struct recognition_result
{
    ISpeechRecognitionResult ISpeechRecognitionResult_iface;
    ISpeechRecognitionResult2 ISpeechRecognitionResult2_iface;
    LONG ref;

    ISpeechRecognitionConstraint *constraint;
    HSTRING text;
};

static inline struct recognition_result *impl_from_ISpeechRecognitionResult( ISpeechRecognitionResult *iface )
{
    return CONTAINING_RECORD(iface, struct recognition_result, ISpeechRecognitionResult_iface);
}

static HRESULT WINAPI recognition_result_QueryInterface( ISpeechRecognitionResult *iface, REFIID iid, void **out )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechRecognitionResult))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionResult_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognitionResult2))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionResult2_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI recognition_result_AddRef( ISpeechRecognitionResult *iface )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI recognition_result_Release( ISpeechRecognitionResult *iface )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if(!ref)
    {
        ISpeechRecognitionConstraint_Release(impl->constraint);
        WindowsDeleteString(impl->text);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI recognition_result_GetIids( ISpeechRecognitionResult *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_GetRuntimeClassName( ISpeechRecognitionResult *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_GetTrustLevel( ISpeechRecognitionResult *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_get_Status( ISpeechRecognitionResult *iface, SpeechRecognitionResultStatus *value )
{
    FIXME("iface %p, operation %p stub!\n", iface, value);
    *value = SpeechRecognitionResultStatus_Success;
    return S_OK;
}

static HRESULT WINAPI recognition_result_get_Text( ISpeechRecognitionResult *iface, HSTRING *value )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);
    TRACE("iface %p, operation %p, text: %s.\n", iface, value, debugstr_hstring(impl->text));
    return WindowsDuplicateString(impl->text, value);
}

static HRESULT WINAPI recognition_result_get_Confidence( ISpeechRecognitionResult *iface, SpeechRecognitionConfidence *value )
{
    FIXME("iface %p, operation %p semi stub!\n", iface, value);
    *value = SpeechRecognitionConfidence_High;
    return S_OK;
}

static HRESULT WINAPI recognition_result_get_SemanticInterpretation( ISpeechRecognitionResult *iface,
                                                                     ISpeechRecognitionSemanticInterpretation **value )
{
    FIXME("iface %p, operation %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_GetAlternates( ISpeechRecognitionResult *iface,
                                                        UINT32 max_amount,
                                                        IVectorView_SpeechRecognitionResult **results )
{
    IVector_IInspectable *vector;
    struct vector_iids constraints_iids =
    {
        .iterable = &IID_IVectorView_SpeechRecognitionResult,
        .iterator = &IID_IVectorView_SpeechRecognitionResult,
        .vector = &IID_IVector_IInspectable,
        .view = &IID_IVectorView_SpeechRecognitionResult,
    };

    FIXME("iface %p, max_amount %u, results %p stub!\n", iface, max_amount, results);

    vector_inspectable_create(&constraints_iids, (IVector_IInspectable **)&vector);
    IVector_IInspectable_GetView(vector, (IVectorView_IInspectable **)results);
    IVector_IInspectable_Release(vector);
    return S_OK;
}

static HRESULT WINAPI recognition_result_get_Constraint( ISpeechRecognitionResult *iface, ISpeechRecognitionConstraint **value )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);
    TRACE("iface %p, operation %p.\n", iface, value);
    ISpeechRecognitionConstraint_AddRef((*value = impl->constraint));
    return S_OK;
}

static HRESULT WINAPI recognition_result_get_RulePath(ISpeechRecognitionResult *iface, IVectorView_HSTRING **value)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_get_RawConfidence(ISpeechRecognitionResult *iface, DOUBLE *value)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static const struct ISpeechRecognitionResultVtbl recognition_result_vtbl =
{
    /* IUnknown methods */
    recognition_result_QueryInterface,
    recognition_result_AddRef,
    recognition_result_Release,
    /* IInspectable methods */
    recognition_result_GetIids,
    recognition_result_GetRuntimeClassName,
    recognition_result_GetTrustLevel,
    /* ISpeechRecognitionResult methods */
    recognition_result_get_Status,
    recognition_result_get_Text,
    recognition_result_get_Confidence,
    recognition_result_get_SemanticInterpretation,
    recognition_result_GetAlternates,
    recognition_result_get_Constraint,
    recognition_result_get_RulePath,
    recognition_result_get_RawConfidence
};

DEFINE_IINSPECTABLE(recognition_result2, ISpeechRecognitionResult2, struct recognition_result, ISpeechRecognitionResult_iface)

static HRESULT WINAPI recognition_result2_get_PhraseStartTime(ISpeechRecognitionResult2 *iface, DateTime *value)
{
    DateTime dt = { .UniversalTime = 0 };
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = dt;
    return S_OK;
}


static HRESULT WINAPI recognition_result2_get_PhraseDuration(ISpeechRecognitionResult2 *iface, TimeSpan *value)
{
    TimeSpan ts = { .Duration = 50000000LL }; /* Use 5 seconds as stub value. */
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = ts;
    return S_OK;
}

static const struct ISpeechRecognitionResult2Vtbl recognition_result2_vtbl =
{
    /* IUnknown methods */
    recognition_result2_QueryInterface,
    recognition_result2_AddRef,
    recognition_result2_Release,
    /* IInspectable methods */
    recognition_result2_GetIids,
    recognition_result2_GetRuntimeClassName,
    recognition_result2_GetTrustLevel,
    /* ISpeechRecognitionResult2 methods */
    recognition_result2_get_PhraseStartTime,
    recognition_result2_get_PhraseDuration
};

static HRESULT WINAPI recognition_result_create(ISpeechRecognitionConstraint *constraint,
                                                HSTRING result_text,
                                                ISpeechRecognitionResult **out)
{
    struct recognition_result *impl;

    TRACE("out %p.\n", out);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->ISpeechRecognitionResult_iface.lpVtbl = &recognition_result_vtbl;
    impl->ISpeechRecognitionResult2_iface.lpVtbl = &recognition_result2_vtbl;
    impl->ref = 1;

    if (constraint) ISpeechRecognitionConstraint_AddRef((impl->constraint = constraint));
    WindowsDuplicateString(result_text, &impl->text);

    *out = &impl->ISpeechRecognitionResult_iface;
    return S_OK;
}

struct recognition_result_event_args
{
    ISpeechContinuousRecognitionResultGeneratedEventArgs ISpeechContinuousRecognitionResultGeneratedEventArgs_iface;
    LONG ref;

    ISpeechRecognitionResult *result;
};

static inline struct recognition_result_event_args *impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface )
{
    return CONTAINING_RECORD(iface, struct recognition_result_event_args, ISpeechContinuousRecognitionResultGeneratedEventArgs_iface);
}

static HRESULT WINAPI recognition_result_event_args_QueryInterface( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface, REFIID iid, void **out )
{
    struct recognition_result_event_args *impl = impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechContinuousRecognitionResultGeneratedEventArgs))
    {
        IInspectable_AddRef((*out = &impl->ISpeechContinuousRecognitionResultGeneratedEventArgs_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI recognition_result_event_args_AddRef( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface )
{
    struct recognition_result_event_args *impl = impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI recognition_result_event_args_Release( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface )
{
    struct recognition_result_event_args *impl = impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        if (impl->result) ISpeechRecognitionResult_Release(impl->result);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI recognition_result_event_args_GetIids( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_event_args_GetRuntimeClassName( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_event_args_GetTrustLevel( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_event_args_get_Result( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface,
                                                                ISpeechRecognitionResult **value )
{
    struct recognition_result_event_args *impl = impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs(iface);
    FIXME("iface %p value %p stub!\n", iface, value);
    return ISpeechRecognitionResult_AddRef((*value = impl->result));
}

static const struct ISpeechContinuousRecognitionResultGeneratedEventArgsVtbl recognition_result_event_args_vtbl =
{
    /* IUnknown methods */
    recognition_result_event_args_QueryInterface,
    recognition_result_event_args_AddRef,
    recognition_result_event_args_Release,
    /* IInspectable methods */
    recognition_result_event_args_GetIids,
    recognition_result_event_args_GetRuntimeClassName,
    recognition_result_event_args_GetTrustLevel,
    /* ISpeechContinuousRecognitionResultGeneratedEventArgs methods */
    recognition_result_event_args_get_Result
};

static HRESULT WINAPI recognition_result_event_args_create( ISpeechRecognitionResult *result,
                                                            ISpeechContinuousRecognitionResultGeneratedEventArgs **out )
{
    struct recognition_result_event_args *impl;

    TRACE("out %p.\n", out);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->ISpeechContinuousRecognitionResultGeneratedEventArgs_iface.lpVtbl = &recognition_result_event_args_vtbl;
    impl->ref = 1;
    if (result) ISpeechRecognitionResult_AddRef((impl->result = result));

    *out = &impl->ISpeechContinuousRecognitionResultGeneratedEventArgs_iface;
    return S_OK;
}

/*
 *
 * ISpeechRecognitionCompilationResult
 *
 */

struct compilation_result
{
    ISpeechRecognitionCompilationResult ISpeechRecognitionCompilationResult_iface;
    LONG ref;

    SpeechRecognitionResultStatus status;
};

static inline struct compilation_result *impl_from_ISpeechRecognitionCompilationResult( ISpeechRecognitionCompilationResult *iface )
{
    return CONTAINING_RECORD(iface, struct compilation_result, ISpeechRecognitionCompilationResult_iface);
}

static HRESULT WINAPI compilation_result_QueryInterface( ISpeechRecognitionCompilationResult *iface, REFIID iid, void **out )
{
    struct compilation_result *impl = impl_from_ISpeechRecognitionCompilationResult(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechRecognitionCompilationResult))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionCompilationResult_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI compilation_result_AddRef( ISpeechRecognitionCompilationResult *iface )
{
    struct compilation_result *impl = impl_from_ISpeechRecognitionCompilationResult(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI compilation_result_Release( ISpeechRecognitionCompilationResult *iface )
{
    struct compilation_result *impl = impl_from_ISpeechRecognitionCompilationResult(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
        free(impl);

    return ref;
}

static HRESULT WINAPI compilation_result_GetIids( ISpeechRecognitionCompilationResult *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI compilation_result_GetRuntimeClassName( ISpeechRecognitionCompilationResult *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI compilation_result_GetTrustLevel( ISpeechRecognitionCompilationResult *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI compilation_result_get_Status( ISpeechRecognitionCompilationResult *iface, SpeechRecognitionResultStatus *value )
{
    struct compilation_result *impl = impl_from_ISpeechRecognitionCompilationResult(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    *value = impl->status;
    return S_OK;
}

static const struct ISpeechRecognitionCompilationResultVtbl compilation_result_vtbl =
{
    /* IUnknown methods */
    compilation_result_QueryInterface,
    compilation_result_AddRef,
    compilation_result_Release,
    /* IInspectable methods */
    compilation_result_GetIids,
    compilation_result_GetRuntimeClassName,
    compilation_result_GetTrustLevel,
    /* ISpeechRecognitionCompilationResult methods */
    compilation_result_get_Status
};


static HRESULT WINAPI compilation_result_create( SpeechRecognitionResultStatus status, ISpeechRecognitionCompilationResult **out )
{
    struct compilation_result *impl;

    TRACE("out %p.\n", out);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->ISpeechRecognitionCompilationResult_iface.lpVtbl = &compilation_result_vtbl;
    impl->ref = 1;
    impl->status = status;

    *out = &impl->ISpeechRecognitionCompilationResult_iface;
    TRACE("created %p\n", *out);
    return S_OK;
}

/*
 *
 * SpeechContinuousRecognitionSession
 *
 */

struct session
{
    ISpeechContinuousRecognitionSession ISpeechContinuousRecognitionSession_iface;
    LONG ref;

    IVectorView_IInspectable *constraints;

    struct list completed_handlers;
    struct list result_handlers;

    IAudioClient *audio_client;
    IAudioCaptureClient *capture_client;

    vosk_instance vosk_instance;

    HANDLE session_thread;
    HANDLE session_paused_event, session_resume_event, audio_buf_event;
    BOOLEAN session_running, session_paused;
    CRITICAL_SECTION cs;
};

/*
 *
 * ISpeechContinuousRecognitionSession
 *
 */

static inline struct session *impl_from_ISpeechContinuousRecognitionSession( ISpeechContinuousRecognitionSession *iface )
{
    return CONTAINING_RECORD(iface, struct session, ISpeechContinuousRecognitionSession_iface);
}

static DWORD CALLBACK session_thread_cb( void *arg )
{
    const char *delim = "\"";
    ISpeechContinuousRecognitionSession *iface = arg;
    IIterable_IInspectable *iterable = NULL;
    IIterator_IInspectable *iterator = NULL;
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    struct recognize_audio_params recog_params;
    struct get_result_params result_params;
    BOOLEAN running, paused = FALSE;
    BOOL available = FALSE;
    UINT32 frame_count, frames_available, tmp_buf_offset = 0, i;
    BYTE *audio_buf, *tmp_buf;
    WCHAR *recg_text;
    char *str, *save_ptr;
    DWORD flags;

    IAudioClient_GetBufferSize(impl->audio_client, &frame_count);
    FIXME("frame_count %u.\n", frame_count);

    tmp_buf = malloc(sizeof(*tmp_buf) * frame_count * 2); /* *2 because our audio frames have 16bit depth. */

    EnterCriticalSection(&impl->cs);
    running = impl->session_running;
    LeaveCriticalSection(&impl->cs);

    TRACE("constraints %p\n", impl->constraints);
    IVectorView_IInspectable_QueryInterface(impl->constraints, &IID_IIterable_ISpeechRecognitionConstraint, (void **)&iterable);

    while (running)
    {
        EnterCriticalSection(&impl->cs);
        paused = impl->session_paused;
        running = impl->session_running;
        LeaveCriticalSection(&impl->cs);

        if (paused)
        {
            IAudioClient_Stop(impl->audio_client);
            TRACE("Paused!\n");
            SetEvent(impl->session_paused_event);

            WaitForSingleObject(impl->session_resume_event, INFINITE);
            IAudioClient_Start(impl->audio_client);
        }

        if (WaitForSingleObject(impl->audio_buf_event, 500) != WAIT_OBJECT_0)
            continue;

        while (IAudioCaptureClient_GetBuffer(impl->capture_client, &audio_buf, &frames_available, &flags, NULL, NULL) == S_OK
               && tmp_buf_offset < (frame_count * 2))
        {
            memcpy(tmp_buf + tmp_buf_offset, audio_buf, frames_available * 2);
            tmp_buf_offset += (frames_available * 2); /* *2 because our audio frames have 16bit depth. */

            IAudioCaptureClient_ReleaseBuffer(impl->capture_client, frames_available);
        }

        recog_params.instance = impl->vosk_instance;
        recog_params.samples = tmp_buf;
        recog_params.samples_size = tmp_buf_offset;

        tmp_buf_offset = 0;

        if (NT_ERROR(VOSK_CALL(recognize_audio, &recog_params)))
            continue;

        if (recog_params.status < 1)
            continue;

        TRACE("Vosk recognized something!\n");

        result_params.instance = impl->vosk_instance;
        result_params.buf = NULL;
        result_params.buf_len = 1000;
        do
        {
            free(result_params.buf);
            result_params.buf = malloc(result_params.buf_len * sizeof(*result_params.buf));
        } while (VOSK_CALL(get_result, &result_params) == STATUS_BUFFER_TOO_SMALL);

        TRACE("Got \n%s\n", result_params.buf);

        for (i = 0, str = strtok_s(result_params.buf, delim, &save_ptr); i < 3 && str != NULL; i++, str = strtok_s(NULL, delim, &save_ptr))
        {}

        if (str == NULL || !strcmp(str, "\n}"))
            continue;

        recg_text = calloc(strlen(str) + 1, sizeof(WCHAR));
        MultiByteToWideChar(CP_UTF8, 0, str, -1, recg_text, strlen(str) + 1);

        IIterable_IInspectable_First(iterable, &iterator);
        for (IIterator_IInspectable_get_HasCurrent(iterator, &available); available; IIterator_IInspectable_MoveNext(iterator, &available))
        {
            ISpeechRecognitionListConstraint *list_constraint = NULL;
            ISpeechRecognitionConstraint *constraint = NULL;
            IVector_HSTRING *commands = NULL;
            IIterable_HSTRING *commands_iterable;
            IIterator_HSTRING *commands_iterator;
            BOOL available2 = FALSE, has_command = FALSE;
            HSTRING hstr;

            IIterator_IInspectable_get_Current(iterator, (IInspectable **)&constraint);
            ISpeechRecognitionConstraint_QueryInterface(constraint, &IID_ISpeechRecognitionListConstraint, (void**)&list_constraint);
            ISpeechRecognitionListConstraint_get_Commands(list_constraint, &commands);
            ISpeechRecognitionListConstraint_Release(list_constraint);
            IVector_HSTRING_QueryInterface(commands, &IID_IIterable_HSTRING, (void **)&commands_iterable);
            IIterable_HSTRING_First(commands_iterable, &commands_iterator);
            IIterable_HSTRING_Release(commands_iterable);

            for (IIterator_HSTRING_get_HasCurrent(commands_iterator, &available2); available2; IIterator_HSTRING_MoveNext(commands_iterator, &available2))
            {
                const WCHAR *cmd_string;
                WCHAR *cmd_string_lwr;

                IIterator_HSTRING_get_Current(commands_iterator, &hstr);
                cmd_string = WindowsGetStringRawBuffer(hstr, NULL);
                cmd_string_lwr = wcsdup(cmd_string);
                WindowsDeleteString(hstr);

                wcslwr(cmd_string_lwr);
                wcslwr(recg_text);

                TRACE("Comparing cmd_string_lwr %s to recg_text %s.\n", debugstr_w(cmd_string_lwr), debugstr_w(recg_text));

                if (!wcscmp(recg_text, cmd_string_lwr))
                {
                    has_command = TRUE;
                    free(cmd_string_lwr);
                    break;
                }

                free(cmd_string_lwr);
            }

            IIterator_HSTRING_Release(commands_iterator);

            if (has_command)
            {
                ISpeechRecognitionResult *result;
                ISpeechContinuousRecognitionResultGeneratedEventArgs *event_args;
                TRACE("constraint %p has recg_text %s.\n", constraint, debugstr_w(recg_text));

                recognition_result_create(constraint, hstr, &result);
                WindowsDeleteString(hstr); /* We kept hstr alive on purpose in the last for loop. */
                recognition_result_event_args_create(result, &event_args);

                typed_event_handlers_notify(&impl->result_handlers,
                                            (IInspectable *)&impl->ISpeechContinuousRecognitionSession_iface,
                                            (IInspectable *)event_args);

                ISpeechContinuousRecognitionResultGeneratedEventArgs_Release(event_args);
                ISpeechRecognitionResult_Release(result);
            }

            ISpeechRecognitionConstraint_Release(constraint);
        }

        free(recg_text);
        IIterator_IInspectable_Release(iterator);
    }

    IIterable_IInspectable_Release(iterable);
    free(tmp_buf);

    return 0;
}

static HRESULT WINAPI shutdown_session_thread( ISpeechContinuousRecognitionSession *iface )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);

    SetEvent(impl->session_resume_event); /* Set the resume even just in case the session was paused. */
    WaitForSingleObject(impl->session_thread, INFINITE);
    CloseHandle(impl->session_thread);
    impl->session_thread = NULL;
    IAudioClient_Stop(impl->audio_client);

    return S_OK;
}

static HRESULT WINAPI session_QueryInterface( ISpeechContinuousRecognitionSession *iface, REFIID iid, void **out )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ISpeechContinuousRecognitionSession))
    {
        IInspectable_AddRef((*out = &impl->ISpeechContinuousRecognitionSession_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI session_AddRef( ISpeechContinuousRecognitionSession *iface )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI session_Release( ISpeechContinuousRecognitionSession *iface )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        struct vosk_release_params vosk_release_params;
        BOOLEAN running = FALSE;

        EnterCriticalSection(&impl->cs);
        running = impl->session_running;
        LeaveCriticalSection(&impl->cs);

        if (running) shutdown_session_thread(iface);

        typed_event_handlers_clear(&impl->completed_handlers);
        typed_event_handlers_clear(&impl->result_handlers);
        IAudioCaptureClient_Release(impl->capture_client);
        IAudioClient_Release(impl->audio_client);

        vosk_release_params.instance = impl->vosk_instance;
        VOSK_CALL(release, &vosk_release_params);

        CloseHandle(impl->session_paused_event);
        CloseHandle(impl->session_resume_event);
        DeleteCriticalSection(&impl->cs);

        free(impl);
    }

    return ref;
}

static HRESULT WINAPI session_GetIids( ISpeechContinuousRecognitionSession *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_GetRuntimeClassName( ISpeechContinuousRecognitionSession *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_GetTrustLevel( ISpeechContinuousRecognitionSession *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_get_AutoStopSilenceTimeout( ISpeechContinuousRecognitionSession *iface, TimeSpan *value )
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_set_AutoStopSilenceTimeout( ISpeechContinuousRecognitionSession *iface, TimeSpan value )
{
    FIXME("iface %p, value %#I64x stub!\n", iface, value.Duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI start_callback( IInspectable *invoker )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession((ISpeechContinuousRecognitionSession *)invoker);
    HRESULT hr = S_OK;

    impl->session_running = TRUE;
    IAudioClient_Start(impl->audio_client);
    impl->session_thread = CreateThread(NULL, 0, session_thread_cb, impl, 0, NULL);

    return hr;
}

static HRESULT WINAPI session_StartAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    HRESULT hr = S_OK;

    FIXME("iface %p, action %p stub!\n", iface, action);

    EnterCriticalSection(&impl->cs);
    if (!impl->session_thread && !impl->session_running && !impl->session_paused)
    {
        hr = async_action_create((IInspectable *)iface, start_callback, action);
    }
    else hr = 0x80131509;
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI session_StartWithModeAsync( ISpeechContinuousRecognitionSession *iface,
                                                  SpeechContinuousRecognitionMode mode,
                                                  IAsyncAction **action )
{
    FIXME("iface %p, mode %u, action %p stub!\n", iface, mode, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI stop_callback( IInspectable *invoker )
{
    shutdown_session_thread((ISpeechContinuousRecognitionSession *)invoker);
    return S_OK;
}

static HRESULT WINAPI session_StopAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    HRESULT hr = 0x80131509;

    FIXME("iface %p, action %p stub!\n", iface, action);

    EnterCriticalSection(&impl->cs);
    if (impl->session_thread && impl->session_running && !impl->session_paused)
    {
        impl->session_running = FALSE;
        LeaveCriticalSection(&impl->cs);

        hr = async_action_create((IInspectable *)iface, stop_callback, action);
    }
    else LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI session_CancelAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    FIXME("iface %p, action %p stub!\n", iface, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI pause_callback( IInspectable *invoker )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession((ISpeechContinuousRecognitionSession *)invoker);

    TRACE("invoker %p\n", invoker);

    WaitForSingleObject(impl->session_paused_event, INFINITE);
    return S_OK;
}

static HRESULT WINAPI session_PauseAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    HRESULT hr = 0x80131509;

    FIXME("iface %p, action %p stub!\n", iface, action);

    EnterCriticalSection(&impl->cs);
    if (impl->session_thread && impl->session_running && !impl->session_paused)
    {
        impl->session_paused = TRUE;
        LeaveCriticalSection(&impl->cs);

        hr = async_action_create((IInspectable *)iface, pause_callback, action);
    }
    else LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI session_Resume( ISpeechContinuousRecognitionSession *iface )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    HRESULT hr = 0x80131509;

    FIXME("iface %p stub!\n", iface);

    EnterCriticalSection(&impl->cs);
    if (impl->session_thread && impl->session_running && impl->session_paused)
    {
        impl->session_paused = FALSE;
        LeaveCriticalSection(&impl->cs);

        SetEvent(impl->session_resume_event);
        hr = S_OK;
    }
    else LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI session_add_Completed( ISpeechContinuousRecognitionSession *iface,
                                             ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionCompletedEventArgs *handler,
                                             EventRegistrationToken *token )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, handler %p, token %p.\n", iface, handler, token);
    if (!handler) return E_INVALIDARG;
    return typed_event_handlers_append(&impl->completed_handlers, (ITypedEventHandler_IInspectable_IInspectable *)handler, token);
}

static HRESULT WINAPI session_remove_Completed( ISpeechContinuousRecognitionSession *iface, EventRegistrationToken token )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, token.value %#I64x.\n", iface, token.value);
    return typed_event_handlers_remove(&impl->completed_handlers, &token);
}

static HRESULT WINAPI session_add_ResultGenerated( ISpeechContinuousRecognitionSession *iface,
                                                   ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionResultGeneratedEventArgs *handler,
                                                   EventRegistrationToken *token)
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, handler %p, token %p.\n", iface, handler, token);
    if (!handler) return E_INVALIDARG;
    return typed_event_handlers_append(&impl->result_handlers, (ITypedEventHandler_IInspectable_IInspectable *)handler, token);
}

static HRESULT WINAPI session_remove_ResultGenerated( ISpeechContinuousRecognitionSession *iface, EventRegistrationToken token )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, token.value %#I64x.\n", iface, token.value);
    return typed_event_handlers_remove(&impl->result_handlers, &token);
}

static const struct ISpeechContinuousRecognitionSessionVtbl session_vtbl =
{
    /* IUnknown methods */
    session_QueryInterface,
    session_AddRef,
    session_Release,
    /* IInspectable methods */
    session_GetIids,
    session_GetRuntimeClassName,
    session_GetTrustLevel,
    /* ISpeechContinuousRecognitionSession methods */
    session_get_AutoStopSilenceTimeout,
    session_set_AutoStopSilenceTimeout,
    session_StartAsync,
    session_StartWithModeAsync,
    session_StopAsync,
    session_CancelAsync,
    session_PauseAsync,
    session_Resume,
    session_add_Completed,
    session_remove_Completed,
    session_add_ResultGenerated,
    session_remove_ResultGenerated
};

/*
 *
 * SpeechRecognizer
 *
 */

struct recognizer
{
    ISpeechRecognizer ISpeechRecognizer_iface;
    IClosable IClosable_iface;
    ISpeechRecognizer2 ISpeechRecognizer2_iface;
    LONG ref;

    ISpeechContinuousRecognitionSession *session;
    IVector_ISpeechRecognitionConstraint *constraints;
};

/*
 *
 * ISpeechRecognizer
 *
 */

static inline struct recognizer *impl_from_ISpeechRecognizer( ISpeechRecognizer *iface )
{
    return CONTAINING_RECORD(iface, struct recognizer, ISpeechRecognizer_iface);
}

static HRESULT WINAPI recognizer_QueryInterface( ISpeechRecognizer *iface, REFIID iid, void **out )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechRecognizer))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizer_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IClosable))
    {
        IInspectable_AddRef((*out = &impl->IClosable_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizer2))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizer2_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI recognizer_AddRef( ISpeechRecognizer *iface )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI recognizer_Release( ISpeechRecognizer *iface )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        ISpeechContinuousRecognitionSession_Release(impl->session);
        IVector_ISpeechRecognitionConstraint_Release(impl->constraints);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI recognizer_GetIids( ISpeechRecognizer *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_GetRuntimeClassName( ISpeechRecognizer *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_GetTrustLevel( ISpeechRecognizer *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_Constraints( ISpeechRecognizer *iface, IVector_ISpeechRecognitionConstraint **vector )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);
    TRACE("iface %p, operation %p.\n", iface, vector);
    IVector_ISpeechRecognitionConstraint_AddRef((*vector = impl->constraints));
    return S_OK;
}

static HRESULT WINAPI recognizer_get_CurrentLanguage( ISpeechRecognizer *iface, ILanguage **language )
{
    FIXME("iface %p, operation %p stub!\n", iface, language);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_Timeouts( ISpeechRecognizer *iface, ISpeechRecognizerTimeouts **timeouts )
{
    FIXME("iface %p, operation %p stub!\n", iface, timeouts);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_UIOptions( ISpeechRecognizer *iface, ISpeechRecognizerUIOptions **options )
{
    FIXME("iface %p, operation %p stub!\n", iface, options);
    return E_NOTIMPL;
}

static HRESULT WINAPI compile_callback( IInspectable *invoker, IInspectable **result )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer((ISpeechRecognizer *)invoker);
    struct session *session = impl_from_ISpeechContinuousRecognitionSession(impl->session);

    if (session->constraints) IVectorView_IInspectable_Release(session->constraints);

    IVector_IInspectable_GetView((IVector_IInspectable *)impl->constraints, &session->constraints);
    
    return compilation_result_create(SpeechRecognitionResultStatus_Success, (ISpeechRecognitionCompilationResult **) result);
}

static HRESULT WINAPI recognizer_CompileConstraintsAsync( ISpeechRecognizer *iface,
                                                          IAsyncOperation_SpeechRecognitionCompilationResult **operation )
{
    IAsyncOperation_IInspectable **value = (IAsyncOperation_IInspectable **)operation;
    FIXME("iface %p, operation %p semi-stub!\n", iface, operation);
    return async_operation_inspectable_create(&IID_IAsyncOperation_SpeechRecognitionCompilationResult, (IInspectable *)iface, compile_callback, value);
}

static HRESULT WINAPI recognizer_RecognizeAsync( ISpeechRecognizer *iface,
                                                 IAsyncOperation_SpeechRecognitionResult **operation )
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_RecognizeWithUIAsync( ISpeechRecognizer *iface,
                                                       IAsyncOperation_SpeechRecognitionResult **operation )
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_add_RecognitionQualityDegrading( ISpeechRecognizer *iface,
                                                                  ITypedEventHandler_SpeechRecognizer_SpeechRecognitionQualityDegradingEventArgs *handler,
                                                                  EventRegistrationToken *token )
{
    FIXME("iface %p, operation %p, token %p, stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_remove_RecognitionQualityDegrading( ISpeechRecognizer *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token.value %#I64x, stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_add_StateChanged( ISpeechRecognizer *iface,
                                                   ITypedEventHandler_SpeechRecognizer_SpeechRecognizerStateChangedEventArgs *handler,
                                                   EventRegistrationToken *token )
{
    FIXME("iface %p, operation %p, token %p, stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_remove_StateChanged( ISpeechRecognizer *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token.value %#I64x, stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerVtbl speech_recognizer_vtbl =
{
    /* IUnknown methods */
    recognizer_QueryInterface,
    recognizer_AddRef,
    recognizer_Release,
    /* IInspectable methods */
    recognizer_GetIids,
    recognizer_GetRuntimeClassName,
    recognizer_GetTrustLevel,
    /* ISpeechRecognizer methods */
    recognizer_get_CurrentLanguage,
    recognizer_get_Constraints,
    recognizer_get_Timeouts,
    recognizer_get_UIOptions,
    recognizer_CompileConstraintsAsync,
    recognizer_RecognizeAsync,
    recognizer_RecognizeWithUIAsync,
    recognizer_add_RecognitionQualityDegrading,
    recognizer_remove_RecognitionQualityDegrading,
    recognizer_add_StateChanged,
    recognizer_remove_StateChanged,
};

/*
 *
 * IClosable
 *
 */

DEFINE_IINSPECTABLE(closable, IClosable, struct recognizer, ISpeechRecognizer_iface)

static HRESULT WINAPI closable_Close( IClosable *iface )
{
    FIXME("iface %p stub.\n", iface);
    return E_NOTIMPL;
}

static const struct IClosableVtbl closable_vtbl =
{
    /* IUnknown methods */
    closable_QueryInterface,
    closable_AddRef,
    closable_Release,
    /* IInspectable methods */
    closable_GetIids,
    closable_GetRuntimeClassName,
    closable_GetTrustLevel,
    /* IClosable methods */
    closable_Close,
};

/*
 *
 * ISpeechRecognizer2
 *
 */

DEFINE_IINSPECTABLE(recognizer2, ISpeechRecognizer2, struct recognizer, ISpeechRecognizer_iface)

static HRESULT WINAPI recognizer2_get_ContinuousRecognitionSession( ISpeechRecognizer2 *iface,
                                                                    ISpeechContinuousRecognitionSession **session )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer2(iface);
    TRACE("iface %p, session %p.\n", iface, session);
    ISpeechContinuousRecognitionSession_QueryInterface(impl->session, &IID_ISpeechContinuousRecognitionSession, (void **)session);
    return S_OK;
}

static HRESULT WINAPI recognizer2_get_State( ISpeechRecognizer2 *iface, SpeechRecognizerState *state )
{
    FIXME("iface %p, state %p stub!\n", iface, state);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer2_StopRecognitionAsync( ISpeechRecognizer2 *iface, IAsyncAction **action )
{
    FIXME("iface %p, action %p stub!\n", iface, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer2_add_HypothesisGenerated( ISpeechRecognizer2 *iface,
                                                           ITypedEventHandler_SpeechRecognizer_SpeechRecognitionHypothesisGeneratedEventArgs *handler,
                                                           EventRegistrationToken *token )
{
    FIXME("iface %p, operation %p, token %p, stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer2_remove_HypothesisGenerated( ISpeechRecognizer2 *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token.value %#I64x, stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizer2Vtbl speech_recognizer2_vtbl =
{
    /* IUnknown methods */
    recognizer2_QueryInterface,
    recognizer2_AddRef,
    recognizer2_Release,
    /* IInspectable methods */
    recognizer2_GetIids,
    recognizer2_GetRuntimeClassName,
    recognizer2_GetTrustLevel,
    /* ISpeechRecognizer2 methods */
    recognizer2_get_ContinuousRecognitionSession,
    recognizer2_get_State,
    recognizer2_StopRecognitionAsync,
    recognizer2_add_HypothesisGenerated,
    recognizer2_remove_HypothesisGenerated,
};

/*
 *
 * Statics for SpeechRecognizer
 *
 */

struct recognizer_statics
{
    IActivationFactory IActivationFactory_iface;
    ISpeechRecognizerFactory ISpeechRecognizerFactory_iface;
    ISpeechRecognizerStatics ISpeechRecognizerStatics_iface;
    ISpeechRecognizerStatics2 ISpeechRecognizerStatics2_iface;
    LONG ref;
};

/*
 *
 * IActivationFactory
 *
 */

static inline struct recognizer_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD(iface, struct recognizer_statics, IActivationFactory_iface);
}

static HRESULT WINAPI activation_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IInspectable_AddRef((*out = &impl->IActivationFactory_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizerFactory))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizerFactory_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizerStatics))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizerStatics_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizerStatics2))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizerStatics2_iface));
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef( IActivationFactory *iface )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI activation_factory_Release( IActivationFactory *iface )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);
    TRACE("iface %p, instance %p.\n", iface, instance);
    return ISpeechRecognizerFactory_Create(&impl->ISpeechRecognizerFactory_iface, NULL, (ISpeechRecognizer **)instance);
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    /* IUnknown methods */
    activation_factory_QueryInterface,
    activation_factory_AddRef,
    activation_factory_Release,
    /* IInspectable methods */
    activation_factory_GetIids,
    activation_factory_GetRuntimeClassName,
    activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    activation_factory_ActivateInstance,
};

/*
 *
 * ISpeechRecognizerFactory
 *
 */

DEFINE_IINSPECTABLE(recognizer_factory, ISpeechRecognizerFactory, struct recognizer_statics, IActivationFactory_iface)

static HRESULT init_audio_capture(IAudioClient **audio_client, IAudioCaptureClient **capture_client, HANDLE audio_buf_event)
{
    IMMDeviceEnumerator *mm_enum = NULL;
    IMMDevice *mm_device = NULL;
    WAVEFORMATEX *wfx = NULL;
    WCHAR *str = NULL;
    const REFERENCE_TIME buffer_duration = 5000000; /* 0.5 second */
    HRESULT hr = S_OK;

    if (FAILED(hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mm_enum)))
        goto cleanup;

    if (FAILED(hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mm_enum, eCapture, eMultimedia, &mm_device)))
        goto cleanup;

    if (FAILED(hr = IMMDevice_Activate(mm_device, &IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void**)audio_client)))
        goto cleanup;

    if (SUCCEEDED(hr = IMMDevice_GetId(mm_device, &str)))
    {
        TRACE("Device ID:%s\n", debugstr_w(str));
        CoTaskMemFree(str);
    }

    if (FAILED(hr = IAudioClient_GetMixFormat(*audio_client, (WAVEFORMATEX**) &wfx)))
        goto cleanup;

    wfx->wFormatTag = WAVE_FORMAT_PCM;
    wfx->nChannels = 1;
    wfx->nSamplesPerSec = 16000;
    wfx->nBlockAlign = 2;
    wfx->wBitsPerSample = 16;
    wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
    TRACE("tag %u, channels %u, samples %lu, bits %u, align %u.\n", wfx->wFormatTag,
                                                                    wfx->nChannels,
                                                                    wfx->nSamplesPerSec,
                                                                    wfx->wBitsPerSample,
                                                                    wfx->nBlockAlign);

    if (FAILED(hr = IAudioClient_Initialize(*audio_client, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, buffer_duration, 0, wfx, NULL)))
    {
        IAudioClient_Release(*audio_client);
        goto cleanup;
    }

    if (FAILED(hr = IAudioClient_SetEventHandle(*audio_client, audio_buf_event)))
    {
        IAudioClient_Release(*audio_client);
        goto cleanup;
    }

    if (FAILED(hr = IAudioClient_GetService(*audio_client, &IID_IAudioCaptureClient, (void**)capture_client)))
    {
        IAudioClient_Release(*audio_client);
        goto cleanup;
    }

cleanup:
    if (mm_device)
        IMMDevice_Release(mm_device);
    if (mm_enum)
        IMMDeviceEnumerator_Release(mm_enum);
    return hr;
}

static HRESULT WINAPI recognizer_factory_Create( ISpeechRecognizerFactory *iface, ILanguage *language, ISpeechRecognizer **speechrecognizer )
{
    struct recognizer *impl;
    struct session *session;
    struct vosk_create_params vosk_create_params;
    struct vector_iids constraints_iids =
    {
        .iterable = &IID_IIterable_ISpeechRecognitionConstraint,
        .iterator = &IID_IIterator_ISpeechRecognitionConstraint,
        .vector = &IID_IVector_ISpeechRecognitionConstraint,
        .view = &IID_IVectorView_ISpeechRecognitionConstraint,
    };
    HRESULT hr;

    TRACE("iface %p, language %p, speechrecognizer %p.\n", iface, language, speechrecognizer);

    *speechrecognizer = NULL;

    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;
    if (!(session = calloc(1, sizeof(*session))))
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    if (language)
        FIXME("language parameter unused. Stub!\n");

    session->ISpeechContinuousRecognitionSession_iface.lpVtbl = &session_vtbl;
    session->ref = 1;
    session->audio_buf_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    session->session_paused_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    session->session_resume_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!session->session_paused_event || !session->session_resume_event || !session->audio_buf_event)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    list_init(&session->completed_handlers);
    list_init(&session->result_handlers);
    InitializeCriticalSection(&session->cs);
    session->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": recognition_session.cs");

    if (FAILED(hr = init_audio_capture(&session->audio_client, &session->capture_client, session->audio_buf_event)))
        goto error;

    vosk_create_params.model_path = getenv("PROTON_VOSK_MODEL_PATH");
    vosk_create_params.sample_rate = 16000.f;
    vosk_create_params.instance = &session->vosk_instance;

    if (FAILED(hr = HRESULT_FROM_NT(VOSK_CALL(create, &vosk_create_params))))
        goto error;

    impl->ISpeechRecognizer_iface.lpVtbl = &speech_recognizer_vtbl;
    impl->IClosable_iface.lpVtbl = &closable_vtbl;
    impl->ISpeechRecognizer2_iface.lpVtbl = &speech_recognizer2_vtbl;
    impl->session = &session->ISpeechContinuousRecognitionSession_iface;
    impl->ref = 1;
    if (FAILED(hr = vector_inspectable_create(&constraints_iids, (IVector_IInspectable**)&impl->constraints)))
        goto error;

    TRACE("created SpeechRecognizer %p.\n", impl);

    *speechrecognizer = &impl->ISpeechRecognizer_iface;
    return S_OK;

error:
    if (session->capture_client) IAudioCaptureClient_Release(session->capture_client);
    if (session->audio_client) IAudioClient_Release(session->audio_client);
    CloseHandle(session->session_resume_event);
    CloseHandle(session->session_paused_event);
    free(session);
    free(impl);

    return hr;
}

static const struct ISpeechRecognizerFactoryVtbl speech_recognizer_factory_vtbl =
{
    /* IUnknown methods */
    recognizer_factory_QueryInterface,
    recognizer_factory_AddRef,
    recognizer_factory_Release,
    /* IInspectable methods */
    recognizer_factory_GetIids,
    recognizer_factory_GetRuntimeClassName,
    recognizer_factory_GetTrustLevel,
    /* ISpeechRecognizerFactory methods */
    recognizer_factory_Create
};

/*
 *
 * ISpeechRecognizerStatics
 *
 */

DEFINE_IINSPECTABLE(statics, ISpeechRecognizerStatics, struct recognizer_statics, IActivationFactory_iface)

static HRESULT WINAPI statics_get_SystemSpeechLanguage( ISpeechRecognizerStatics *iface, ILanguage **language )
{
    FIXME("iface %p, language %p stub!\n", iface, language);
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_get_SupportedTopicLanguages( ISpeechRecognizerStatics *iface, IVectorView_Language **languages )
{
    FIXME("iface %p, languages %p stub!\n", iface, languages);
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_get_SupportedGrammarLanguages( ISpeechRecognizerStatics *iface, IVectorView_Language **languages )
{
    FIXME("iface %p, languages %p stub!\n", iface, languages);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerStaticsVtbl speech_recognizer_statics_vtbl =
{
    /* IUnknown methods */
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* ISpeechRecognizerStatics2 methods */
    statics_get_SystemSpeechLanguage,
    statics_get_SupportedTopicLanguages,
    statics_get_SupportedGrammarLanguages
};

/*
 *
 * ISpeechRecognizerStatics2
 *
 */

DEFINE_IINSPECTABLE(statics2, ISpeechRecognizerStatics2, struct recognizer_statics, IActivationFactory_iface)

static HRESULT WINAPI statics2_TrySetSystemSpeechLanguageAsync( ISpeechRecognizerStatics2 *iface,
                                                                ILanguage *language,
                                                                IAsyncOperation_boolean **operation)
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerStatics2Vtbl speech_recognizer_statics2_vtbl =
{
    /* IUnknown methods */
    statics2_QueryInterface,
    statics2_AddRef,
    statics2_Release,
    /* IInspectable methods */
    statics2_GetIids,
    statics2_GetRuntimeClassName,
    statics2_GetTrustLevel,
    /* ISpeechRecognizerStatics2 methods */
    statics2_TrySetSystemSpeechLanguageAsync,
};

static struct recognizer_statics recognizer_statics =
{
    .IActivationFactory_iface = {&activation_factory_vtbl},
    .ISpeechRecognizerFactory_iface = {&speech_recognizer_factory_vtbl},
    .ISpeechRecognizerStatics_iface = {&speech_recognizer_statics_vtbl},
    .ISpeechRecognizerStatics2_iface = {&speech_recognizer_statics2_vtbl},
    .ref = 1
};

IActivationFactory *recognizer_factory = &recognizer_statics.IActivationFactory_iface;
