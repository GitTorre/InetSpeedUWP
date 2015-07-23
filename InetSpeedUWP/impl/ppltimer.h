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
* ppltimer.h
*
* Parallel Patterns Library Power Pack
*
* For the latest on this and related APIs, please see http://pplpp.codeplex.com.
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#pragma once
#include <agents.h>

#include <functional>
#include <chrono>
#include <memory>
#include <atomic>
#include <ppl.h>


namespace pplpp
{
    namespace details
    {
        class TimerPoolImpl
        {
            class TimerWithCallback
            {
                concurrency::timer<int> m_timer;
                concurrency::call<int> m_callback;
                std::function<void(bool)> m_userCallback;
                concurrency::cancellation_token m_token;
                concurrency::cancellation_token_registration m_reg;
                std::atomic<bool> m_fired, m_constructed;

            public:
                TimerWithCallback(std::chrono::milliseconds timeout, std::function<void(bool) > callback, concurrency::cancellation_token token) : m_timer(static_cast<int>(timeout.count()), 0, NULL, false), 
                    m_callback([this] (int) mutable { this->timerCallback(true); }), m_userCallback(callback), m_token(token), m_fired(false), m_constructed(false)
                {
                    m_timer.link_target(&m_callback);
                    m_timer.start();
                    if (token != concurrency::cancellation_token::none())
                        m_reg = token.register_callback([this] ()mutable {this->timerCallback(false); });
                    m_constructed = true;
                }

                void timerCallback(bool firedbytimer)
                {
                    if (m_fired.exchange(true))
                        return;

                    if (firedbytimer)
                    {
                        if (m_token != concurrency::cancellation_token::none())
                            m_token.deregister_callback(m_reg);
                    }
                    else
                        m_timer.stop();

                    struct AsyncDeletor
                    {
                        TimerWithCallback *m_target;
                        AsyncDeletor(TimerWithCallback *target) : m_target(target) {}
                        ~AsyncDeletor()
                        {
                            auto t = m_target;
                            concurrency::create_task([=] {
                                while (!t->m_constructed); // busy wait for constructor completion
                                delete t;
                            });						
                        }
                    } deletor(this);

                    m_userCallback(firedbytimer);
                }
            };
        public:
            void queue_timer_callback(std::chrono::milliseconds timeout, std::function<void(bool)> callback, concurrency::cancellation_token token = concurrency::cancellation_token::none())
            {
                // Memory will be cleand by itself
                new TimerWithCallback(timeout, callback, token);
            }
        };
    }
    typedef details::TimerPoolImpl timer_pool_t;
};
