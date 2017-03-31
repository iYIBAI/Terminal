/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "screenInfoUiaProvider.hpp"
#include "screenInfo.hpp"

#include "windowUiaProvider.hpp"
#include "window.hpp"

#include "UiaTextRange.hpp"

// A helper function to create a SafeArray Version of an int array of a specified length
SAFEARRAY* BuildIntSafeArray(_In_reads_(length) const int* const data, _In_ int const length)
{
    SAFEARRAY *psa = SafeArrayCreateVector(VT_I4, 0, length);
    if (psa != nullptr)
    {
        for (long i = 0; i < length; i++)
        {
            if (FAILED(SafeArrayPutElement(psa, &i, (void *)&(data[i]))))
            {
                SafeArrayDestroy(psa);
                psa = nullptr;
                break;
            }
        }
    }

    return psa;
}

ScreenInfoUiaProvider::ScreenInfoUiaProvider(_In_ Window* const pParent, _In_ SCREEN_INFORMATION* const pScreenInfo) :
    _pWindow(pParent),
    _pScreenInfo(pScreenInfo),
    _cRefs(1)
{
}

ScreenInfoUiaProvider::~ScreenInfoUiaProvider()
{

}

#pragma region IUnknown

IFACEMETHODIMP_(ULONG) ScreenInfoUiaProvider::AddRef()
{
    return InterlockedIncrement(&_cRefs);
}

IFACEMETHODIMP_(ULONG) ScreenInfoUiaProvider::Release()
{
    long val = InterlockedDecrement(&_cRefs);
    if (val == 0)
    {
        delete this;
    }
    return val;
}

IFACEMETHODIMP ScreenInfoUiaProvider::QueryInterface(_In_ REFIID riid, _COM_Outptr_result_maybenull_ void** ppInterface)
{
    if (riid == __uuidof(IUnknown))
    {
        *ppInterface = static_cast<IRawElementProviderSimple*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderSimple))
    {
        *ppInterface = static_cast<IRawElementProviderSimple*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderFragment))
    {
        *ppInterface = static_cast<IRawElementProviderFragment*>(this);
    }
    else if (riid == __uuidof(ITextProvider))
    {
        *ppInterface = static_cast<ITextProvider*>(this);
    }
    else
    {
        *ppInterface = nullptr;
        return E_NOINTERFACE;
    }

    (static_cast<IUnknown*>(*ppInterface))->AddRef();

    return S_OK;
}

#pragma endregion

#pragma region IRawElementProviderSimple

// Implementation of IRawElementProviderSimple::get_ProviderOptions.
// Gets UI Automation provider options.
IFACEMETHODIMP ScreenInfoUiaProvider::get_ProviderOptions(_Out_ ProviderOptions* pOptions)
{
    *pOptions = ProviderOptions_ServerSideProvider;
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PatternProvider.
// Gets the object that supports ISelectionPattern.
IFACEMETHODIMP ScreenInfoUiaProvider::GetPatternProvider(_In_ PATTERNID patternId, _COM_Outptr_result_maybenull_ IUnknown** ppInterface)
{
    *ppInterface = nullptr;

    // TODO: MSFT: 7960168 - insert patterns here
    if (patternId == UIA_TextPatternId)
    {
        *ppInterface = static_cast<ITextProvider*>(this);
        (*ppInterface)->AddRef();
    }

    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PropertyValue.
// Gets custom properties.
IFACEMETHODIMP ScreenInfoUiaProvider::GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT* pVariant)
{
    pVariant->vt = VT_EMPTY;

    // Returning the default will leave the property as the default
    // so we only really need to touch it for the properties we want to implement
    if (propertyId == UIA_ControlTypePropertyId)
    {
        // This control is the Document control type, implying that it is
        // a complex document that supports text pattern
        pVariant->vt = VT_I4;
        pVariant->lVal = UIA_DocumentControlTypeId;
    }
    else if (propertyId == UIA_NamePropertyId)
    {
        // TODO: MSFT: 7960168 - These strings should be localized text in the final UIA work
        pVariant->bstrVal = SysAllocString(L"Text Area");
        if (pVariant->bstrVal != nullptr)
        {
            pVariant->vt = VT_BSTR;
        }
    }
    else if (propertyId == UIA_AutomationIdPropertyId)
    {
        pVariant->bstrVal = SysAllocString(L"Text Area");
        if (pVariant->bstrVal != nullptr)
        {
            pVariant->vt = VT_BSTR;
        }
    }
    else if (propertyId == UIA_IsControlElementPropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_IsContentElementPropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_IsKeyboardFocusablePropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_FALSE;
    }
    else if (propertyId == UIA_HasKeyboardFocusPropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_FALSE;
    }
    else if (propertyId == UIA_ProviderDescriptionPropertyId)
    {
        pVariant->bstrVal = SysAllocString(L"Microsoft Console Host: Screen Information Text Area");
        if (pVariant->bstrVal != nullptr)
        {
            pVariant->vt = VT_BSTR;
        }
    }

    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_HostRawElementProvider(_COM_Outptr_result_maybenull_ IRawElementProviderSimple** ppProvider)
{
    *ppProvider = nullptr;

    return S_OK;
}
#pragma endregion

#pragma region IRawElementProviderFragment

IFACEMETHODIMP ScreenInfoUiaProvider::Navigate(_In_ NavigateDirection direction, _COM_Outptr_result_maybenull_ IRawElementProviderFragment** ppProvider)
{
    *ppProvider = nullptr;

    if (direction == NavigateDirection_Parent)
    {
        *ppProvider = new WindowUiaProvider(_pWindow);
        RETURN_IF_NULL_ALLOC(*ppProvider);
    }

    // For the other directions the default of nullptr is correct
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::GetRuntimeId(_Outptr_result_maybenull_ SAFEARRAY** ppRuntimeId)
{
    // Root defers this to host, others must implement it...
    *ppRuntimeId = nullptr;

    // AppendRuntimeId is a magic Number that tells UIAutomation to Append its own Runtime ID(From the HWND)
    int rId[] = { UiaAppendRuntimeId, -1 };
    // BuildIntSafeArray is a custom function to hide the SafeArray creation
    *ppRuntimeId = BuildIntSafeArray(rId, 2);
    RETURN_IF_NULL_ALLOC(*ppRuntimeId);

    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_BoundingRectangle(_Out_ UiaRect* pRect)
{
    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, _pWindow);

    RECT const rc = _pWindow->GetWindowRect();

    pRect->left = rc.left;
    pRect->top = rc.top;
    pRect->width = rc.right - rc.left;
    pRect->height = rc.bottom - rc.top;

    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY** ppRoots)
{
    *ppRoots = nullptr;
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::SetFocus()
{
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_FragmentRoot(_COM_Outptr_result_maybenull_ IRawElementProviderFragmentRoot** ppProvider)
{
    *ppProvider = new WindowUiaProvider(_pWindow);
    RETURN_IF_NULL_ALLOC(*ppProvider);
    return S_OK;
}

#pragma endregion

#pragma region ITextProvider

IFACEMETHODIMP ScreenInfoUiaProvider::GetSelection(SAFEARRAY** ppRetVal)
{
    /*
    Selection selection = Selection::Instance();
    if (!selection.IsInSelectionState())
    {
        // TODO
    }
    */
    UNREFERENCED_PARAMETER(ppRetVal);
    return E_NOTIMPL;
}

IFACEMETHODIMP ScreenInfoUiaProvider::GetVisibleRanges(SAFEARRAY** ppRetVal)
{
    const SMALL_RECT viewport = _pScreenInfo->GetBufferViewport();
    TEXT_BUFFER_INFO* outputBuffer = _pScreenInfo->TextInfo;
    const FontInfo currentFont = *outputBuffer->GetCurrentFont();
    const COORD currentFontSize = currentFont.GetUnscaledSize();
    const size_t charWidth = viewport.Right - viewport.Left + 1;

    // make a safe array
    const size_t rowCount = viewport.Bottom - viewport.Top + 1;
    *ppRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(rowCount));
    if (*ppRetVal == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    // stuff each visible line in the safearray
    for (size_t i = 0; i < rowCount; ++i)
    {
        UiaTextRange* range = new UiaTextRange(this, outputBuffer, TextUnit_Line, currentFontSize, viewport.Top + i, i, viewport);
        this->AddRef();
        LONG currentIndex = static_cast<LONG>(i);
        HRESULT hr = SafeArrayPutElement(*ppRetVal, &currentIndex, (void*)range);
        if (FAILED(hr))
        {
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return hr;
        }
/*
        ROW* row = outputBuffer->GetRowByOffset(static_cast<UINT>(i));
		std::wstring wstr = L"";
		if (row->CharRow.ContainsText())
		{
			wstr = std::wstring(row->CharRow.Chars + row->CharRow.Left,
								row->CharRow.Chars + row->CharRow.Right);
		}
        size_t charTop = viewport.Top + i;

        std::unique_ptr<UiaTextRange> range = std::make_unique<UiaTextRange>(this, wstr, charTop, charWidth, currentFontSize);
        this->AddRef();

        LONG currentIndex = static_cast<LONG>(i);
        HRESULT hr = SafeArrayPutElement(*ppRetVal, &currentIndex, range.get());
        range.release();
        if (FAILED(hr))
        {
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return hr;
        }
        */
    }
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::RangeFromChild(IRawElementProviderSimple* childElement,
                                                 ITextRangeProvider** ppRetVal)
{
    UNREFERENCED_PARAMETER(childElement);
    UNREFERENCED_PARAMETER(ppRetVal);
    return E_NOTIMPL;
}

IFACEMETHODIMP ScreenInfoUiaProvider::RangeFromPoint(UiaPoint point,
                                                  ITextRangeProvider** ppRetVal)
{
    UNREFERENCED_PARAMETER(point);
    UNREFERENCED_PARAMETER(ppRetVal);
    return E_NOTIMPL;
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_DocumentRange(ITextRangeProvider** ppRetVal)
{
    const TEXT_BUFFER_INFO* const pOutputBuffer = _pScreenInfo->TextInfo;
    const ROW* const row = pOutputBuffer->GetFirstRow();
    if (row->CharRow.ContainsText())
    {
        ; // TODO
    }
    UNREFERENCED_PARAMETER(ppRetVal);
    return E_NOTIMPL;
}

// TODO change this when selection is supported
IFACEMETHODIMP ScreenInfoUiaProvider::get_SupportedTextSelection(SupportedTextSelection* pRetVal)
{
    //UNREFERENCED_PARAMETER(pRetVal);
    *pRetVal = SupportedTextSelection::SupportedTextSelection_None;
    //return E_NOTIMPL;
    return S_OK;
}

#pragma endregion
