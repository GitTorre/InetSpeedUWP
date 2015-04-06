/***
* ==++==
*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* winrtcontexcallback.h
*
* Parallel Patterns Library Power Pack
*
* For the latest on this and related APIs, please see http://pplpp.codeplex.com.
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#pragma once
#include <functional>
#ifdef __cplusplus_winrt
#include <windows.h>
#endif

namespace pplpp { namespace details
{
    class ContextCallback
    {
#ifdef __cplusplus_winrt

    public:
        ~ContextCallback()
        {
            reset();
        }

        ContextCallback()
        {
            m_context = nullptr;
        }

        ContextCallback(const ContextCallback& src)
        {
            assign(src.m_context);
        }

        ContextCallback(ContextCallback&& src)
        {
            m_context = src.m_context;
            src.m_context = nullptr;
        }

        ContextCallback& operator=(const ContextCallback& src)
        {
            if (this != &src)
            {
                reset();
                assign(src.m_context);
            }
            return *this;
        }

        ContextCallback& operator=(ContextCallback&& src)
        {
            if (this != &src)
            {
                reset();
                m_context = src.m_context;
                src.m_context = nullptr;
            }
            return *this;
        }

        void capture()
        {
            reset();
            if (isCurrentSTA())
            {
                HRESULT hr = CoGetObjectContext(IID_IContextCallback, reinterpret_cast<void **>(&m_context));
                if (FAILED(hr))
                    m_context = nullptr;
            }
        }
        
        void callInContext(std::function<void ()> func) const
        {
            if (m_context == nullptr)
            {
                func();
            }
            else
            {
                ComCallData callData;
                ZeroMemory(&callData, sizeof(callData));
                callData.pUserDefined = reinterpret_cast<void *>(&func);

                HRESULT hr = m_context->ContextCallback(&_bridge, &callData, IID_ICallbackWithNoReentrancyToApplicationSTA, 5, nullptr);
                if (FAILED(hr))
                {
                    throw Platform::Exception::CreateException(hr);
                }
            }
        }

        bool operator==(const ContextCallback& right) const
        {
            return (m_context == right.m_context);
        }

        bool operator!=(const ContextCallback& right) const
        {
            return !(operator==(right));
        }

    private:

        void reset()
        {
            if (m_context != nullptr)
            {
                m_context->Release();
                m_context = nullptr;
            }
        }

        void assign(IContextCallback *context)
        {
            m_context = context;
            if (m_context != nullptr)
            {
                m_context->AddRef();
            }
        }

        static HRESULT __stdcall _bridge(ComCallData *_PParam)
        {
            std::function<void()> *pFunc = reinterpret_cast<std::function<void()> *>(_PParam->pUserDefined);
            (*pFunc)();
            return S_OK;
        }

        // Returns the origin information for the caller (runtime / Windows Runtime apartment as far as task continuations need know)
        bool isCurrentSTA()
        {
            APTTYPE _AptType;
            APTTYPEQUALIFIER _AptTypeQualifier;

            HRESULT hr = CoGetApartmentType(&_AptType, &_AptTypeQualifier);
            if (SUCCEEDED(hr))
            {
                // We determine the origin of a task continuation by looking at where .then is called, so we can tell whether
                // to need to marshal the continuation back to the originating apartment. If an STA thread is in executing in
                // a neutral aparment when it schedules a continuation, we will not marshal continuations back to the STA,
                // since variables used within a neutral apartment are expected to be apartment neutral.
                switch(_AptType)
                {
                    case APTTYPE_MAINSTA:
                    case APTTYPE_STA:
                        return true;
                    default:
                        break;
                }
            }
            return false;
        }

        IContextCallback *m_context;
#else
    public:
        void callInContext(std::function<void()> func) const
        {
            func();
        }
        void capture()	{}
        bool operator==(const ContextCallback&) const
        {
            return true;
        }

        bool operator!=(const ContextCallback&) const
        {
            return false;
        }
#endif
    };
}}
