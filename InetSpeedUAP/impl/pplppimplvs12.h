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
* pplppimplvs12.h
*
* Parallel Patterns Library Power Pack
*
* For the latest on this and related APIs, please see http://pplpp.codeplex.com.
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#pragma once

#include <tuple>
#include <ppl.h>
#include <atomic>
#include <type_traits>
namespace pplpp
{
    namespace details
    {
        template<typename Ty>
        struct Data
        {
            Data(size_t t)
                : total(t)
            {
            }

            std::atomic_size_t counter;
            concurrency::task_completion_event<Ty> tce;
            size_t total;
        };

        template<typename TupleType, size_t N>
        struct when_all_iterator
        {
            static void iterate(const TupleType& task_tuple, std::shared_ptr < Data < TupleType >> data)
            {
                auto current_task = std::get<N>(task_tuple);			//get element task
                typedef std::tuple_element<N, TupleType>::type::result_type RetType;

                current_task.then([=](concurrency::task<RetType> continueTask){
                    if (++(data->counter) == data->total)
                        data->tce.set(task_tuple);
                });

                when_all_iterator<TupleType, N - 1>::iterate(task_tuple, data);
            }
        };

        template<typename TupleType>
        struct when_all_iterator<TupleType, -1>
        {
            static void iterate(const TupleType& tuple, std::shared_ptr < Data < TupleType >> data)
            {
                (tuple);
                (data);
            }
        };


        template<typename TupleType, size_t N>
        struct when_any_iterator
        {
            static void iterate(const TupleType& tuple_of_task, std::shared_ptr < Data < TupleType >> data)
            {
                auto current_task = std::get<N>(tuple_of_task);			//get element task
                typedef std::tuple_element<N, TupleType>::type::result_type RetType;

                current_task.then([=](concurrency::task<RetType> continueTask){
                    data->tce.set(tuple_of_task);
                });

                when_any_iterator<TupleType, N - 1>::iterate(tuple_of_task, data);
            }
        };

        template<typename TupleType>
        struct when_any_iterator<TupleType, -1>
        {
            static void iterate(const TupleType& tuple, std::shared_ptr < Data < TupleType >> data)
            {
                (tuple);
                (data);
            }
        };


        template<typename Tuple, std::size_t N>
        struct tuple_taskGen
        {
            template<typename T>
            static auto taskGen(const Tuple& t, const T& resultTuple)
                ->decltype(tuple_taskGen<Tuple, N - 1>::taskGen(t, std::tuple_cat(std::make_tuple(perform(std::get<N>(t))), resultTuple)))
            {
                auto tempTuple = std::tuple_cat(std::make_tuple(perform(std::get<N>(t))), resultTuple);
                return tuple_taskGen<Tuple, N - 1>::taskGen(t, tempTuple);
            }
        };
        template<typename Tuple>
        struct tuple_taskGen<Tuple, -1>
        {
            template<typename T>
            static T taskGen(const Tuple& t, const T& resultTuple)
            {
                (t);
                return resultTuple;
            }
        };


        template<typename T>
        concurrency::task<T> perform(const T& t)
        {
            return concurrency::task_from_result(t);
        }
        template<typename T>
        concurrency::task<T> perform(const concurrency::task<T>& UnwrappedTask)
        {
            return UnwrappedTask;
        }
#if defined (__cplusplus_winrt)
        template<typename T>
        auto perform(Windows::Foundation::IAsyncOperation<T>^ UnwrappedOperation)->decltype(concurrency::create_task(UnwrappedOperation))
        {
            return concurrency::create_task(UnwrappedOperation);
        }

        template<typename T>
        auto perform(T^ UnwrappedOperation)-> decltype(performHelper<T>::get(std::is_base_of<Windows::Foundation::IAsyncInfo, T>(), UnwrappedOperation))
        {
            return performHelper<T>::get(std::is_base_of<Windows::Foundation::IAsyncInfo, T>(), UnwrappedOperation);
        }

        template<typename T>
        struct performHelper
        {
            static auto get(std::true_type, T^ UnwrappedOperation) -> decltype(concurrency::create_task(UnwrappedOperation))
            {
                return concurrency::create_task(UnwrappedOperation);
            }

            static auto  get(std::false_type, T^ UnwrappedOperation) -> decltype(concurrency::task_from_result(UnwrappedOperation))
            {
                return concurrency::task_from_result(UnwrappedOperation);
            }
        };


#endif /* defined (__cplusplus_winrt) */

        //tuple unwrapper
        template<typename Tuple, std::size_t N>
        struct tuple_unwrapper
        {
            template<typename T>
            static auto unwrapping(const Tuple& t, const T& resultTuple)
                ->decltype(tuple_unwrapper<Tuple, N - 1>::unwrapping(t, std::tuple_cat(std::make_tuple(std::get<N>(t).get()), resultTuple)))
            {
                auto tempTuple = std::tuple_cat(std::make_tuple(std::get<N>(t).get()), resultTuple);
                return tuple_unwrapper<Tuple, N - 1>::unwrapping(t, tempTuple);
            }
        };
        template<typename Tuple>
        struct tuple_unwrapper<Tuple, -1>
        {
            template<typename T>
            static T unwrapping(const Tuple& t, const T& resultTuple)
            {
                (t);
                return resultTuple;
            }
        };

        //get the return type
        template<typename Tuple, std::size_t N>
        struct getUnwrapperReturnType
        {
            template<typename T>
            static auto  get(const Tuple& t, const T& resultTuple)
                ->decltype(getUnwrapperReturnType<Tuple, N - 1>::get(t, std::tuple_cat(std::make_tuple(std::tuple_element<N, Tuple>::type::result_type()), resultTuple)))
            {
                auto e = std::tuple_element<N, Tuple>::type::result_type();
                auto t2 = std::tuple_cat(std::make_tuple(e), resultTuple);

                return getUnwrapperReturnType<Tuple, N - 1>::get(t, t2);
            }
        };
        template<typename Tuple>
        struct getUnwrapperReturnType<Tuple, -1>
        {
            template<typename T>
            static T get(const Tuple& t, const T& resultTuple)
            {
                return resultTuple;
            }
        };

        template<typename TypeInputTuple>
        auto unwrappingTupleHelper(const TypeInputTuple& inputTuple)
            ->concurrency::task < decltype(getUnwrapperReturnType<decltype(tuple_taskGen<TypeInputTuple, std::tuple_size<TypeInputTuple>::value - 1>::taskGen(inputTuple, std::tuple<>())),
            std::tuple_size<decltype(tuple_taskGen<TypeInputTuple, std::tuple_size<TypeInputTuple>::value - 1>::taskGen(inputTuple, std::tuple<>()))>::value - 1>::get(tuple_taskGen<TypeInputTuple,
            std::tuple_size<TypeInputTuple>::value - 1>::taskGen(inputTuple, std::tuple<>()), std::tuple<>())) >
        {
            auto tupleTask = tuple_taskGen<TypeInputTuple, std::tuple_size<TypeInputTuple>::value - 1>::taskGen(inputTuple, std::tuple<>());    //tuple of tasks
            auto task_tupleTask = pplpp::when_all(tupleTask);    //task of tuple of tasks

            typedef decltype(tupleTask) TypeTupleTask;
            auto returnTask = task_tupleTask.then([](TypeTupleTask _tuple){
                return tuple_unwrapper<TypeTupleTask, std::tuple_size<TypeTupleTask>::value - 1>::unwrapping(_tuple, std::tuple<>());
            });

            return returnTask;
        }

        template <typename Func>
        class UnwrapperedFunctor
        {
            template <typename Func>
            static std::false_type not_has_arg(const Func &, ...) {}
            template <typename Func>
            static auto not_has_arg(const Func &func, int) -> decltype(func(), std::true_type()) {}

            template<typename HasArgument, typename Func, typename Arg>
            static auto invoke(std::true_type, const Func& f, Arg a)->decltype(details::unwrappingTupleHelper(f(a)))
            {
                return details::unwrappingTupleHelper(f(a));
            }

            template<typename HasArgument, typename Func>
            static auto invoke(std::false_type, const Func& f)->decltype(details::unwrappingTupleHelper(f()))
            {
                return details::unwrappingTupleHelper(f());
            }

            template <typename HasArgument>
            static auto invoke(HasArgument, ...)->int
            {
                return 1;
            }
            Func m_lambda;
        public:
            UnwrapperedFunctor(const Func& lambda)
                : m_lambda(lambda)
            {
            }

            template<typename Arg>
            auto operator()(Arg arg) const-> decltype(invoke<decltype(not_has_arg(m_lambda, 0))>(std::true_type(), m_lambda, arg)) // unwrapped<RetType>
            {
                auto result = invoke<decltype(not_has_arg(m_lambda, 0))>(std::true_type(), m_lambda, arg);
                return result;
            }

            auto operator()() const-> decltype(invoke<decltype(not_has_arg(m_lambda, 0))>(std::false_type(), m_lambda)) // unwrapped<RetType>
            {
                auto result = invoke<decltype(not_has_arg(m_lambda, 0))>(std::false_type(), m_lambda);
                return result;
            }
        };
    } //namespace details


    /// <summary>
    ///     Creates a task that will complete successfully when all of the nonhomogeneous tasks supplied as arguments complete successfully.
    /// </summary>
    /// <typeparam name="... Ty">
    ///     The variadic types for input task type
    /// </typeparam>
    /// <param name="tasks">
    ///     A list of heterogeneous tasks
    /// </param>
    /// <returns>
    ///     A task that completes sucessfully when all of the input tasks have completed successfully.
    ///     If the input tasks are <c>task &lt; T1 &gt; </c> and <c>task &lt; T2 &gt;</c>, 
    ///     the output of this function is <c> task &lt; std::tuple &lt; task &lt; T1 &gt;, task &lt; T2 &gt;&gt &gt;</c>; 
    ///	</return>
    /// <remarks>
    ///     If one of the tasks throws an exception, the returned task will complete early. 
    ///     The exception will be thrown if you call <c>get()</c> or <c>wait()</c> on that task.
    /// </remarks>
    template <typename ... Ty>
    concurrency::task<std::tuple<Ty...>> when_all(Ty... tasks)
    {
        auto task_tuple = std::make_tuple(tasks...);
        return when_all(task_tuple);
    }

    /// <summary>
    ///     Creates a task that will complete successfully when all of the nonhomogeneous tasks supplied as arguments complete successfully.
    /// </summary>
    /// <param name="task_tuple">
    ///     A tuple of heterogenous tasks
    /// </param>
    /// <returns>
    ///	    A task that completes sucessfully when all of the input tasks have completed successfully.
    ///	</return>
    /// <remarks>
    ///     If one of the tasks throws an exception, the returned task will complete early. 
    ///     The exception will be thrown if you call <c>get()</c> or <c>wait()</c> on that task.
    /// </remarks>
    template <typename ... Ty>
    concurrency::task<std::tuple<Ty...>> when_all(const std::tuple<Ty...>& task_tuple)
    {
        auto data = std::make_shared < details::Data < std::tuple<Ty... >> >(std::tuple_size< std::tuple<Ty...> > ::value);
        details::when_all_iterator < std::tuple<Ty...>, std::tuple_size< std::tuple<Ty... > > ::value - 1>::iterate(task_tuple, data);

        return concurrency::create_task(data->tce);
    }

    /// <summary>
    ///     Creates a task that will complete successfully when any of the heterogenous tasks supplied as arguments completes successfully.
    /// </summary>
    /// <typeparam name="... Ty">
    ///     The variadic types for input task type
    /// </typeparam>
    /// <param name="tasks">
    ///     A list of heterogenous tasks
    /// </param>
    /// <returns>
    ///     A task that completes sucessfully when all of the input tasks have completed successfully.
    ///     If the input tasks are <c>task &lt; T1 &gt; </c> and <c>task &lt; T2 &gt;</c>, 
    ///     the output of this function is <c> task &lt; std::tuple &lt; task &lt; T1 &gt;, task &lt; T2 &gt;&gt &gt;</c>; 
    ///	</return>
    /// <remarks>
    ///     If one of the tasks throws an exception, the returned task will complete early. 
    ///     The exception will be thrown if you call <c>get()</c> or <c>wait()</c> on that task.
    /// </remarks>
    template <typename ... Ty>
    concurrency::task<std::tuple<Ty...>> when_any(Ty... tasks)
    {
        auto task_tuple = std::make_tuple(tasks...);
        return when_any(task_tuple);
    }

    /// <summary>
    ///     Creates a task that will complete successfully when any of the heterogenous tasks supplied as arguments completes successfully.
    /// </summary>
    /// <typeparam name="... Ty">
    ///     The variadic types for input task type
    /// </typeparam>
    /// <param name="task_tuple">
    ///     A tuple of heterogenous tasks
    /// </param>
    /// <returns>
    ///     A task that completes sucessfully when any of the input tasks have completed successfully.
    ///     If the input tasks are <c>task &lt; T1 &gt; </c> and <c>task &lt; T2 &gt;</c>, 
    ///     the output of this function is <c> task &lt; std::tuple &lt; task &lt; T1 &gt;, task &lt; T2 &gt;&gt &gt;</c>; 
    ///	</return>
    /// <remarks>
    ///     If one of the tasks throws an exception, the returned task will complete early. 
    ///     The exception will be thrown if you call <c>get()</c> or <c>wait()</c> on that task.
    /// </remarks>
    template <typename ... Ty>
    concurrency::task<std::tuple<Ty...>> when_any(const std::tuple<Ty...>& task_tuple)
    {
        auto data = std::make_shared < details::Data < std::tuple<Ty... >> >(std::tuple_size<std::tuple<Ty...> >::value);
        details::when_any_iterator<std::tuple<Ty...>, std::tuple_size<std::tuple<Ty...> >::value - 1>::iterate(task_tuple, data);

        return concurrency::create_task(data->tce);
    }

    /// <summary>
    ///     Create a functor that unwrapes the return tuple of the input functor.
    ///     We can use this function to pass data in task continuation.
    /// </summary>
    /// <param name="func">
    ///     A functor which returns a tuple of tasks or Async operations.
    /// </param>
    /// <returns>
    ///     A functor which returns a task, and the task returns the unwrapped tuple.
    ///     If the input functor returns std::tuple &lt; task &lt; T &gt;, IAsyncOperation &lt; StorageFile &gt;^&gt;, 
    ///     the return value of this function is a functor which returns std::tuple &lt; T, StorageFile^&gt;, 
    ///	</return>
    /// <remark>
    ///     The return tuple of input functor cannot include task &gt; void &gt;. 
    /// </remark>
    template<typename Func>
    details::UnwrapperedFunctor<Func> unwrap_tuple(const Func& func)
    {
        return details::UnwrapperedFunctor<Func>(func);
    }
}