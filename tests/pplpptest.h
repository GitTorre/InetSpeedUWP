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
* pplpptest.h
*
* Parallel Patterns Library Power Pack
*
* For the latest on this and related APIs, please see http://pplpp.codeplex.com.
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/
namespace pplppTest {

    namespace helper
    {
#ifdef __cplusplus_winrt
        template <typename T>
        concurrency::task_status await(concurrency::task<T> t)
        {
            Windows::UI::Core::CoreWindow^ wnd = Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow;
            Windows::UI::Core::CoreDispatcher^ dispatcher = wnd->Dispatcher;

            concurrency::event ev;
            concurrency::task_status result = concurrency::not_complete;
            std::exception_ptr eptr = nullptr;

            t.then([&](concurrency::task<T> previousTask){
                try
                {
                    // task_canceled will not throw exception
                    result = previousTask.wait();
                }
                catch (...)
                {
                    eptr = std::current_exception();
                }
                ev.set();
            }, concurrency::task_continuation_context::use_arbitrary());

            while (ev.wait(1))
                dispatcher->ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);

            if (eptr)
                std::rethrow_exception(eptr);
            return result;
        }

#else
        template <typename T>
        concurrency::task_status await(concurrency::task<T> t)
        {
            return t.wait();
        }
#endif
    }

    using namespace helper;


#ifdef __cplusplus_winrt

    ref class MyHatType
    {
        int m_value;
    internal:
        MyHatType(int val)
        {
            m_value = val;
        }
        int getValue()
        {
            return m_value;
        }
    };
    TEST_CLASS(TaskWithProgressTest)
    {

        TEST_METHOD(AsyncOpProgressTest)
        {
            concurrency::event ev;
            Windows::Foundation::IAsyncActionWithProgress<MyHatType^> ^action = concurrency::create_async([&](concurrency::progress_reporter<MyHatType^> p) {
                ev.wait();
                for (int i = 1; i <= 100; i++)
                {
                    auto hatProgress = ref new MyHatType(i);
                    p.report(hatProgress);
                }
            });

            pplpp::task_with_progress<void, MyHatType^> tp1 = pplpp::create_task_with_progress(action);

            int sum1 = 0, sum2 = 0;
            tp1.on_progress([&](MyHatType^ progress) {
                sum1 += progress->getValue();
            });
            tp1.on_progress([&](MyHatType^ progress) {
                sum2 += progress->getValue();
            });
            ev.set();

            Assert::AreEqual(true, tp1.is_apartment_aware());
            concurrency::task<void> t1 = tp1;
            await(t1);
            Assert::AreEqual(5050, sum1);
            Assert::AreEqual(5050, sum2);

            ev.reset();
            Windows::Foundation::IAsyncOperationWithProgress<float, int> ^op = concurrency::create_async([&](concurrency::progress_reporter<int> p) {
                ev.wait();
                for (int i = 1; i <= 100; i++)
                    p.report(i);
                return 3.0f;
            });

            pplpp::task_with_progress<float, int> tp2 = pplpp::create_task_with_progress(op);

            sum1 = 0, sum2 = 0;
            tp2.on_progress([&](int progress) {
                sum1 += progress;
            });
            tp2.on_progress([&](int progress) {
                sum2 += progress;
            });
            ev.set();
            Assert::AreEqual(true, tp2.is_apartment_aware());
            concurrency::task<float> t2 = tp2;
            await(t2);
            Assert::AreEqual(5050, sum1);
            Assert::AreEqual(5050, sum2);
            Assert::AreEqual(3.0f, t2.get());
        }

                TEST_METHOD(lambdaProgressTest)
        {
            concurrency::event ev;
            pplpp::task_with_progress<float, int> tp = pplpp::create_task_with_progress([&ev](concurrency::progress_reporter<int> p) {
                ev.wait();
                for (int i = 1; i <= 100; i++)
                    p.report(i);
                return 3.0f;
            });
            auto id = std::this_thread::get_id();
            int sum1 = 0, sum2 = 0;
            tp.on_progress([&](int progress) {
                sum1 += progress;
                Assert::AreEqual(id.hash(), std::this_thread::get_id().hash());
            });
            tp.on_progress([&](int progress) {
                sum2 += progress;
                Assert::AreEqual(id.hash(), std::this_thread::get_id().hash());
            });
            ev.set();
            Assert::AreEqual(false, tp.is_apartment_aware());
            concurrency::task<float> t = tp;
            await(t);
            Assert::AreEqual(5050, sum1);
            Assert::AreEqual(5050, sum2);
            Assert::AreEqual(3.0f, t.get());
        }
    };
#endif

    TEST_CLASS(IterativeTaskTest)
    {
        TEST_METHOD(simpleIterativeTask)
        {
            int counter = 0;
            await(pplpp::create_iterative_task([&] {
                if (counter++ < 5000)
                    return concurrency::create_task([] { return true; });
                else
                    return concurrency::create_task([] { return false; });
            }));
            Assert::AreEqual(5001, counter);
        }
#ifdef __cplusplus_winrt
        TEST_METHOD(STAIterativeTask)
        {
            int counter = 0;
            auto id = std::this_thread::get_id();
            await(pplpp::create_iterative_task([&] {
                Assert::AreEqual(id.hash(), std::this_thread::get_id().hash());
                if (counter++ < 5000)
                    return concurrency::create_task([] { return true; });
                else
                    return concurrency::create_task([] { return false; });

            }, concurrency::task_continuation_context::use_current()));
            Assert::AreEqual(5001, counter);
        }
#endif
        TEST_METHOD(iterativeTaskwithCancellation)
        {
            int counter = 0;
            concurrency::cancellation_token_source cts;
            auto t = pplpp::create_iterative_task([&] {
                if (counter++ < 5000)
                {
                    if (counter == 1000)
                        cts.cancel();
                    return concurrency::create_task([] { return true; });
                }
                else
                    return concurrency::create_task([] { return false; });
            }, concurrency::task_continuation_context::use_default(), cts.get_token());
            Assert::AreEqual(int(await(t)), int(concurrency::canceled));
        }

        TEST_METHOD(iterativeTaskwithException)
        {
            int counter = 0;
            class TestException {};
            await(pplpp::create_iterative_task([&] {
                if (counter++ < 500)
                    return concurrency::create_task([] { return true; });
                else
                    throw TestException();
            }).then([&](concurrency::task<void> t) {
                Assert::ExpectException<TestException>([&] {
                    t.wait();
                });
                Assert::AreEqual(501, counter);
            }));

            counter = 0;
            await(pplpp::create_iterative_task([&] {
                if (counter++ < 1000)
                {
                    return concurrency::create_task([&] {
                        if (counter <= 500)
                            return true;
                        else
                            throw TestException();
                    });
                }
                else
                    throw concurrency::create_task([] { return false; });
            }).then([&](concurrency::task<void> t) {
                Assert::ExpectException<TestException>([&] {
                    t.wait();
                });
                Assert::AreEqual(501, counter);
            }));
        }

    };

    TEST_CLASS(TimerTaskTest)
    {
        TEST_METHOD(simpleTimer)
        {
            using namespace std::chrono;
            system_clock::time_point startTime = system_clock::now();
            await(pplpp::create_timer_task(milliseconds(500)).then([=] {
                auto timeDiff = duration_cast<milliseconds>(system_clock::now() - startTime).count();
                Assert::IsTrue(timeDiff >= 500 && timeDiff < 750);
                // Delay another 500
                return pplpp::create_timer_task(milliseconds(500));
            }).then([=] {
                auto timeDiff = duration_cast<milliseconds>(system_clock::now() - startTime).count();
                Assert::IsTrue(timeDiff >= 1000 && timeDiff < 1500);
            }));
        }

        TEST_METHOD(timerWithCancellation)
        {
            using namespace std::chrono;
            system_clock::time_point startTime = system_clock::now();
            concurrency::cancellation_token_source tcs;
            auto t = pplpp::create_timer_task(seconds(1), tcs.get_token()).then([=](concurrency::task<void> t) {
                auto timeDiff = duration_cast<milliseconds>(system_clock::now() - startTime).count();
                Assert::IsTrue(timeDiff < 100);
                Assert::AreEqual(int(t.wait()), int(concurrency::canceled));

                return pplpp::create_timer_task(seconds(1), tcs.get_token());
            }).then([=] {
                Assert::Fail();
            });
            tcs.cancel();
            Assert::AreEqual(int(await(t)), int(concurrency::canceled));
            auto timeDiff = duration_cast<milliseconds>(system_clock::now() - startTime).count();
            Assert::IsTrue(timeDiff < 100);

        }

        TEST_METHOD(timedCancellationTokenSource)
        {
            using namespace std::chrono;
            system_clock::time_point startTime = system_clock::now();
            pplpp::timed_cancellation_token_source tcs1;
            auto t = pplpp::create_timer_task(seconds(1), tcs1.get_token()).then([=](concurrency::task<void> t) {
                auto timeDiff = duration_cast<milliseconds>(system_clock::now() - startTime).count();
                Assert::IsTrue(timeDiff >= 100 && timeDiff < 300);
                Assert::AreEqual(int(t.wait()), int(concurrency::canceled));
            });
            tcs1.cancel(milliseconds(100));
            await(t);

            startTime = system_clock::now();
            pplpp::timed_cancellation_token_source tcs2;
            t = concurrency::create_task([] {
                volatile int v = 0;
                for (int i = 0; i < 10000000000; i++)
                {
                    v = 0;
                    concurrency::interruption_point();
                    while (v < 100000)
                        ++v;
                }
            }, tcs2.get_token()).then([=](concurrency::task<void> t) {
                auto timeDiff = duration_cast<milliseconds>(system_clock::now() - startTime).count();
                Assert::IsTrue(timeDiff >= 100 && timeDiff < 1000);
                Assert::AreEqual(int(t.wait()), int(concurrency::canceled));
            });
            tcs2.cancel(milliseconds(100));
            await(t);

            startTime = system_clock::now();
            pplpp::timed_cancellation_token_source tcs3;
            t = concurrency::create_task([] {
                volatile int v = 0;
                for (int i = 0; i < 10000000000; i++)
                {
                    v = 0;
                    concurrency::interruption_point();
                    while (v < 100000)
                        ++v;
                }
            }, tcs3.get_token()).then([=](concurrency::task<void> t) {
                auto timeDiff = duration_cast<milliseconds>(system_clock::now() - startTime).count();
                Assert::IsTrue(timeDiff < 200);
                Assert::AreEqual(int(t.wait()), int(concurrency::canceled));
            });
            tcs3.cancel();
            await(t);
        }
    };

#if _MSC_VER >= 1800

    TEST_CLASS(whenallTest)
    {
    public:
        TEST_METHOD(when_all_simpleTaskInput)
        {
            auto pn = std::make_shared<std::atomic_size_t>(0);

            concurrency::task<int> t1([=]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
                return 1;
            });

            concurrency::task<char> t2([pn]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
                return 'c';
            });

            concurrency::task<void> t3([pn]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
            });

            concurrency::task<bool> t4([pn]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
                return true;
            });

            await(pplpp::when_all(t1, t2, t3, t4).then([=](std::tuple<concurrency::task<int>, concurrency::task<char>, 
                concurrency::task<void>, concurrency::task<bool>> t){
                    Assert::AreEqual(std::get<0>(t).get(), 1);
                    Assert::AreEqual(std::get<1>(t).get(), 'c');
                    Assert::AreEqual(std::get<3>(t).get(), true);
                    Assert::IsTrue(*pn == 4000000);
            }));
        }

        TEST_METHOD(when_all_Exception)
        {
            auto pn = std::make_shared<std::atomic_size_t>(0);
            class TestException {};

            concurrency::task<int> t1([=]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
                return 1;
            });
            concurrency::task<void> t2([]{
                throw 1;
            });
            concurrency::task<float> t3([=]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
                return 1.0f;
            });

            concurrency::task<bool> t4([=]()->bool{
                throw std::runtime_error("Test Exception");
            });

            await(pplpp::when_all(t1, t2, t3, t4).then([=](std::tuple < concurrency::task<int>, concurrency::task<void>, 
                    concurrency::task<float>, concurrency::task<bool>> t){
                        Assert::AreEqual(std::get<0>(t).get(), 1);

                        Assert::ExpectException<int>([&]{
                            std::get<1>(t).get();
                        });

                        Assert::AreEqual(std::get<2>(t).get(), 1.0f);

                        Assert::ExpectException<std::runtime_error>([&]{
                            std::get<3>(t).get();
                        });

                        Assert::IsTrue(*pn == 2000000);
            }));
        }


        TEST_METHOD(when_all_tupleInput)
        {
            auto pn = std::make_shared<std::atomic_size_t>(0);
            concurrency::task<int> t1([pn]{
                for (int i = 0; i < 1000000; i++)
                    (*pn)++;
                return 1;
            });

            concurrency::task<char> t2([pn]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
                return 'c';
            });

            auto tuple = std::make_tuple(t1, t2);
            await(pplpp::when_all(tuple).then([=](std::tuple<concurrency::task<int>, concurrency::task <char>> t){
                Assert::IsTrue(*pn == 2000000);
            }));
        }
    };

    TEST_CLASS(whenanyTest)
    {
        TEST_METHOD(when_any_SimpleTest)
        {
            auto pn = std::make_shared<std::atomic_size_t>(0);
            concurrency::task<int> t1([pn]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
                return 1;
            });

            concurrency::task<void> t2([pn]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
            });

            concurrency::task<bool> t3([pn]{
                for (int i = 0; i < 1000000; ++i)
                    (*pn)++;
                return true;
            });

            auto tuple = std::make_tuple(t1, t2, t3);
            await(pplpp::when_any(tuple).then([=](std::tuple<concurrency::task<int>, concurrency::task<void>, 
                concurrency::task<bool>> t){
                    Assert::IsTrue(*pn >= 1000000);
            }));
        }

        TEST_METHOD(when_any_InfinitLoopTask)
        {
            auto pn = std::make_shared<int>(0);
            concurrency::cancellation_token_source cts;

            concurrency::task<int> t1([=]{
                for (int i = 0; i < 10000; ++i)
                {
                    (*pn)++;
                }
                return *pn;
            });
            concurrency::task<void> t2([=]{
                for (int i = 0;;)
                {
                    concurrency::interruption_point();
                    ++i;
                }
            }, cts.get_token());

            await(pplpp::when_any(t1, t2).then([=](std::tuple<concurrency::task<int>, 
                concurrency::task<void>> t){
                    Assert::AreEqual(*pn, 10000);
            }));

            cts.cancel();
        }

        TEST_METHOD(when_any_withException)
        {
            concurrency::task<void> t1([]{
                throw std::runtime_error("Test Exception");
            });

            concurrency::task<int> t2([]{
                int pn = 0;
                for (int i = 0; i < 1000000; ++i)
                {
                    pn++;
                }
                return pn; 
            });

            concurrency::task<float> t3([]()->float{
                throw 1;
            });
          
            await(pplpp::when_any(t1, t2, t3).then([](std::tuple<concurrency::task<void>, concurrency::task<int>, concurrency::task<float>> t){
                Assert::IsTrue(std::get<0>(t).is_done() || std::get<1>(t).is_done() || std::get<2>(t).is_done());

                //catch exceptions
                await(pplpp::when_all(t).then([](std::tuple<concurrency::task<void>, concurrency::task<int>, concurrency::task<float>> _t){
                    Assert::ExpectException<std::runtime_error>([&]{ 
                        std::get<0>(_t).get(); }
                    );
                    Assert::ExpectException<int>([&]{
                        std::get<2>(_t).get();
                    });
                }));
            }));
        }        
    };


    TEST_CLASS(UnwrappingTest)
    {
    public:
        TEST_METHOD(unwrap_tuple)
        {
            auto pn1 = std::make_shared<int>(0);
            auto pn2 = std::make_shared<int>(0);
            auto ts = concurrency::create_task(pplpp::unwrap_tuple([=]{
                bool m = true;
                double n = 2.1;
                concurrency::task<int> t1([=]{
                    for (int i = 0; i < 1000000; i++)
                        (*pn1)++;
                    return *pn1;
                });
                concurrency::task<int> t2([=]{
                    for (int i = 0; i < 10000; i++)
                        (*pn2)++;
                    return *pn2;
                });
                auto tuple = std::make_tuple(m, n, t1, t2);
                return tuple;
            }));

            await(ts.then([](std::tuple<bool, double, int, int> tuple){
                Assert::AreEqual(std::get<0>(tuple), true);
                Assert::AreEqual(std::get<2>(tuple), 1000000);
                Assert::AreEqual(std::get<3>(tuple), 10000);
            }));
        }

        TEST_METHOD(unwrap_tuple_WithException)
        {
            auto pn1 = std::make_shared<int>(0);
            auto ts = concurrency::create_task(pplpp::unwrap_tuple([=]{
                concurrency::task<int> t1([=]{
                    for (int i = 0; i < 1000000; i++)
                        (*pn1)++;
                    return *pn1;
                });

                concurrency::task<bool> t2([=]()->bool{
                    throw std::runtime_error("Test Exception");
                });
                auto tuple = std::make_tuple(t1, t2);

                return tuple;
            }));

            await(ts.then([=](concurrency::task<std::tuple<int, bool>> t){
                Assert::ExpectException<std::runtime_error>([&]{
                    t.get();
                });
            }));
        }
    };

#if defined (__cplusplus_winrt)
    using namespace helper;

    TEST_CLASS(IAsyncOperationUnwrappingTest)
    {
    public:
        TEST_METHOD(unwrap_tuple_AsyncOp_WithoutParam)
        {
            auto folder = KnownFolders::PicturesLibrary;
            String^ fileName = ref new String(L"sample.txt");

            auto getFileTask = concurrency::create_task(pplpp::unwrap_tuple([this, folder, fileName]{
                auto t1 = concurrency::create_task([]{return 1; });
                auto tuple = std::make_tuple(t1, folder->CreateFileAsync(fileName, CreationCollisionOption::OpenIfExists));

                return tuple;
            }));

            auto t = getFileTask.then([&](concurrency::task <std::tuple<int, StorageFile^>> tupleTask){
                auto tuple = tupleTask.get();
                auto v1 = std::get<0>(tuple);
                auto v2 = std::get<1>(tuple);

                Assert::AreEqual(v2->Name, L"sample.txt");
            });

            await(t);
        }

        TEST_METHOD(unwrap_tuple_AsyncOp_WithParam)
        {
            auto folder = KnownFolders::PicturesLibrary;
            String^ fileName = ref new String(L"sample.txt");
            auto getFileTask = concurrency::create_task([this, folder, fileName]{
                return folder->CreateFileAsync(fileName, CreationCollisionOption::OpenIfExists);
            });

            auto openFileTask = getFileTask.then([&](StorageFile^ file){
                return file->OpenAsync(FileAccessMode::Read);
            });

            auto readFileTask = openFileTask.then(pplpp::unwrap_tuple([&](IRandomAccessStream^ istream){
                DataReader^ reader = ref new DataReader(istream);
                auto tuple = std::make_tuple(reader, reader->LoadAsync(static_cast<size_t>(istream->Size)));
                return tuple;
            }));

            auto t = readFileTask.then([this](std::tuple< DataReader^, UINT > tuple){
                auto reader = std::get<0>(tuple);
                UINT bytesRead = std::get<1>(tuple);

                Assert::AreEqual(bytesRead, static_cast<UINT>(3));
            });

            await(t);
        }
    };
#endif /* defined (__cplusplus_winrt) */
#endif  //_MSC_VER >= 1800

}
