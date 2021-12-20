\page Activities

Execution of Asynchronous Tasks may be required for executing SQL or any other data 
queries including HTTP queries to fetch external data. Activities module provides a
generic framework of performing one or more asynchronous operations sequentially or 
parallelly using boost asio io context. All activities perform their corresponding 
operations asynchronously and signal its completion by storing a success or failure
result to a common heterogenous @ref udho::activities::collector "collector". The common 
heterogenous @ref udho::activities::collector "collector" is non-copiable and instantiated 
as shared pointer. The collector is usually accessed through a copiable full or partial 
@ref udho::activities::accessor "accessor" that provides R/W access to a subset of the main storage. 
An activity is defined as a class that provides a no argument `operator()` overload and 
a pair of success `S` and failure `F` result types. An activity `X` inherits
from @ref udho::activities::activity "activity<X, S, F>" which provides two protected 
methods, @ref udho::activities::activity::success "success()" and \ref udho::activities::activity::failure "failure()". 
The `X::operator()` is supposed to start the asynchronous operation by specifying a callback 
function which will be called once the asynchronous operation completes. Depending on whether 
the operation is successful or not, the `finish` callback calls the `success()` or `failure()` 
methods of the base class with the corresponding result data to signal its completion.
One activity may be dependent on completion of one or more other activities prior its 
execution. Hence a \ref udho::activities::combinator "combinator" is used to combine the 
dependencies of an activity `X` and on completion of all its dependencies, it calls or
cancels the invocation of `X::operator()`. e.g. if any of its required dependencies fail
then the activity `X` is canceled.
A \ref udho::activities::subtask "subtask" provides a convenient way of instantiating an
activity and attaching a combinator with it. Instead of instantiating the activity or the
combinator separately, a \ref udho::activities::subtask "subtask" is used. The subtask is 
usually created using the \ref udho::activities::after "after" method.

Example
=========
Following is an example of an activity `A1`. The activity starts some async operation from 
its `operator()()`. It uses `A1::finished` as a callback which is supposed to be called once
that *something* operation finishes. From `A1::finished` it constructs a success result `s`
that is passed to the protected method `success()`.
@code {.cpp}
struct success_t{
    int _value;
};
struct failure_t{
    int _reason;
};
struct A1: activities::activity<A1, success_t, failure_t>{
    using activity_type = activities::activity<A<N>, success_t, failure_t>;
    template <typename CollectorT>
    A(CollectorT& collector): activity_type(collector){}
    void operator()(){
        something.start(); // start async operation
        something.async_wait(std::bind(&A1::finished, activity_type::self()));
    }
    void finished(){
        success_t s;
        s._value = 42;
        activity_type::success(s);
    }
};
@endcode
Similarly we can construct multiple such activities. \ref udho::activities::after "after" with 
no arguments creates the root subtask that does not include a combinator as there are no dependencies. 
Rest of the subtasks includes a combinator to combine their dependencies.
@code 
namespace activities = udho::activities;
auto collector = activities::collect<A0,A1,A2,A3,A4>(ctx);
auto a0 = activities::after()       .perform<A0>(collector);
auto a1 = activities::after(a0)     .perform<A1>(collector);
auto a2 = activities::after(a0)     .perform<A2>(collector);
auto a3 = activities::after(a1, a2) .perform<A3>(collector);
auto a4 = activities::after(a3)     .perform<A4>(collector);
@endcode 
Once `a4` finishes the final callback is called with a full accessor which can access the state and 
result of all other activities.
@code 
auto a_finished = activities::after(a4).finish(collector, [ctx, &test_run](const activities::accessor<A0, A1, A2, A3, A4>& data){
    std::cout << data.completed<A0>() << std::endl; // prints true if completed
    std::cout << data.completed<A1>() << std::endl; // prints true if completed
    std::cout << data.completed<A2>() << std::endl; // prints true if completed
    std::cout << data.completed<A3>() << std::endl; // prints true if completed
    std::cout << data.completed<A4>() << std::endl; // prints true if completed
    A1S s = data.success<A1>(); // get the success data of A1
});
@endcode 
Anatomy
=========
The anatomy of a graph of async activities is shown in the following figure. In this example 
eight activities A1, A2, A3, B1, B2, C1, D1 and D2 are to be executed. So the heterogenous 
\ref udho::activities::collector "collector<ContextT, A1, A2, A3, B1, B2, C1, D1, D2>" collects
results of all these eight activities. The result may include success as well as failure result
as yielded by an activity. The first three activities have no dependencies, hence run parallelly. 
Activity B1 requires both A1 and A2 to complete before it can start. Similarly B2 requires A2 and 
A3 to complete before it can start. `A1` inherits from \ref udho::activities::activity "activity<A1, A1S, A1F>" 
(assuming that `A1S` and `A1F` are two classes intended to denote the success and failure result 
of A1). To signal successful completion `A1::operator()()` calls protected method `activity<A1, A1S, A1F>::success(s)` 
where `s` is an instance of `A1S`. Each of the activities contain a partial accessor to the actual 
collector. The `success()` method stores the success result `s` to the collector through the accessor
internally.
A 2-1 combinator combines A1 and A2 and once both of them completes it calls the `B1::operator()()`. 
A subtask consists of an activity and an appropriate combinator to combine all its dependebcies. Once
all activities finish the final callback is called with a full accessor. The final callback can access
the result and state of all previous activities using that accessor. 
@image html activities-anatomy.png 

Overview
=========

Following is a brief overview of important constructs in the activity module.

Collector
---------

Different activities using different success and failure data will be
involved in the subtask graph. A subtask or even an activity may need to
use the data fetched by its dependencies in order to execute its action.
Hence a heterogenous collector collects the data submited by the
`success` or `failure` methods, which can be accessed through an
accessor. In the following example `data` is a collector that collects
success or failure data of three activities `A1`, `A2`, and `A3`
respectively. Usually all activities involved in the subtask graph
should be passed as a template parameter of `collect`

\code
auto data = udho::activities::collect<A1, A2, A3>(ctx, "Any string name for the collector");
\endcode

However under many circumstances the entire collector is not required to
be accessed. Instead a subset of what has been collected are accessed.
So an `accessor<...>` is created from that collector.

\code
udho::activities::accessor<A1, A2> access(data);
if(!access.failed<A1>()){
    A1SData pre = access.success<A1>();
}
\endcode

The collector has to be passed to the constructor of all activities.

Activity
--------

An `activity` is the definition of the job that has to be performed. A
`subtask` is an instantiation of an `activity`. Definition of an
activity consists of two types `SuccessDataT`, `FailureDataT` and a no
argument function call operarator `operator()()` overload. A `subtask`
on the other hand instantiates an `activity` by specifying its data and
execution dependencies thus establishing an order of invocation. The
philosophy is that the activity is reusable while the subtask reuses
that to model different use cases. The activity is invoked through its
`operator()()` overload after all its prior activities have completed
(dependencies are specified by the subtask). The `operator()()` must
call either `success()` or `failure()` methods either synchronously or
asynchronously with `SuccessDataT` or `FailureDataT` in order to signal
its completion. An activity `A` subclasses from
`udho::activity<A, SuccessA, FailureA>` which defines the `success()`
and `failure()` methods.

In the following example the success and failure data for activity A1
are `A1SData` and `A1FData` respectively. `A1` starts a timer for 5
seconds. Once the timer finishes the value 42 is set as the success
value and the activity finishes.

\code
struct A1SData{
    int value;
};

struct A1FData{
    int reason;
};

struct A1: udho::activities::activity<A1, A1SData, A1FData>{
    typedef udho::activities::activity<A1, A1SData, A1FData> base;

    boost::asio::deadline_timer _timer;

    template <typename CollectorT>
    A1(CollectorT c, boost::asio::io_context& io): base(c), _timer(io){}

    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(5));
        _timer.async_wait(boost::bind(&A1::finished, self(), boost::asio::placeholders::error));
    }

    void finished(const boost::system::error_code& e){
        A1SData data;
        data.value = 42; // Fetched 42 from somewhere
        success(data); // successful
    }
};
\endcode

Subtask
-------

Activities serves the purpose of defining an action. however to action
is supposed to fit in some use case depending on the application needs.
The application needs may require a specific order of invocation of
these activities. So that sufficient data is collected before some
activity is invoked.

In the above mentioned task graph, Both A2 and A3 depends on A1. Hence
A2 and A3 may start as soon as A1 completes. A4 depends on both A2 and
A3 and cannot start until both A2 and A3 completes. In this situation A2
and A3 can access the data fetched by A1 activity because that has
already been completed (succeded or failed) before A2 or A3 is invoked.
Similarly A4 can access the data collected by A1, A2, A3 that has
completed before A4 has been invoked.

`udho::activities::perform` describes the above subtask graph as shown
in the following code block.

\code
auto data = udho::activities::collect<A1, A2i, A3i>(ctx, "A");

auto t1 = udho::activities::perform<A1>::with(data, io);
auto t2 = udho::activities::perform<A2>::require<A1>::with(data, io).after(t1);
auto t3 = udho::activities::perform<A3>::require<A1>::with(data, io).after(t1);
auto t4 = udho::activities::perform<A4>::require<A2, A3>::with(data, io).after(t2).after(t3);
\endcode

The above statements using `perform` are equivalent to the following.

\code
auto data = udho::activities::collect<A1, A2i, A3i>(ctx, "A");

auto t1 = udho::activities::subtask<A1>::with(data, io);
auto t2 = udho::activities::subtask<A2, A1>::with(data, io).after(t1);
auto t3 = udho::activities::subtask<A3, A1>::with(data, io).after(t1);
auto t4 = udho::activities::subtask<A4, A2, A3>::with(data, io).after(t2).after(t3);
\endcode

With this arrangement Success/Failure data of A1 is available to A2 and
A3 before it starts execution. To access that data the constructors on
A2 and A3 may create an accessor as shown below.

\code
struct A2: udho::activities::activity<A2, A2SData, A2FData>{
    typedef udho::activities::activity<A2, A2SData, A2FData> base;

    boost::asio::deadline_timer _timer;
    udho::activities::accessor<A1> _accessor;       // An accessor to access success/failure data of A1

    template <typename CollectorT>
    A2(CollectorT c, boost::asio::io_context& io): base(c), _timer(io), _accessor(c){}

    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(10));
        _timer.async_wait(boost::bind(&A2::finished, self(), boost::asio::placeholders::error));
    }

    void finished(const boost::system::error_code& err){
        if(!err && !_accessor.failed<A1>()){        // check whether A1 has failed or not
            A1SData pre = _accessor.success<A1>();  // access the success data collected from A1
            A2SData data;
            data.value = pre.value + 2;             // creates its own success data
            success(data);                          // signal completion with success
        }
    }
};
\endcode

However in the above mentioned implementation A2 always depends on A1.
In an use case where A2 depends on some other activity or even executed
independently won't be feasible because A2 will still have an accessor
to A1 and it will try to extract the success data of A1 irrespective of
the graph. On the other hand, A2 may still need some data that has to be
set in order to perform the activity. that data may be derived from A1
or from some other activity or through some constructor argument.

\code
struct A2i: udho::activities::activity<A2i, A2SData, A2FData>{
    typedef udho::activities::activity<A2i, A2SData, A2FData> base;

    int prevalue;
    boost::asio::deadline_timer _timer;

    template <typename CollectorT>
    A2i(CollectorT c, boost::asio::io_context& io, int p): base(c), prevalue(p), _timer(io){}

    template <typename CollectorT>
    A2i(CollectorT c, boost::asio::io_context& io): base(c), _timer(io){}

    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(10));
        _timer.async_wait(boost::bind(&A2i::finished, self(), boost::asio::placeholders::error));
    }

    void finished(const boost::system::error_code& err){
        if(!err){
            A2SData data;
            data.value = prevalue + 2;
            success(data);
        }
    }
};
\endcode

In the above example `prevalue` is that piece of information that is
required by `A2i` in order to perform its activity. It doesn't care who
provides that information, whether its A1, or some other activity. `A2i`
has a constructor that takes that `prevalue` as an argument which can be
used when `prevalue` is known at the time of construction, or when `A2i`
is the starting activity in the graph.

\code
auto t2 = udho::activities::perform<A2i>::require<A1>::with(data, io).after(t1).prepare([data](A2i& a2i){
    udho::activities::accessor<A1> a1_access(data);
    A1SData pre = a1_access.success<A1>();
    a2i.prevalue = pre.value;
});
\endcode

In the above code block that activity `A2` is prepared through its
reference passed as the argument right after all its dependencies (only
A1 in this case) completes by the callback provided in the `prepare`
function. The collected `data` is captured inside the lambda function
from which the `A1` success data is accessed.

Final
-----

Finally we would like do something once the entire subtask graph has
completed in a similar fasion. In the following example we do not
perform `A4`. Instead we execute the following piece of code once both
`A2i` and `A3i` completes.

\code
udho::activities::require<A2i, A3i>::with(data).exec([ctx, name](const udho::activities::accessor<A1, A2i, A3i>& d) mutable{
    std::cout << "Final begin" << std::endl;

    int sum = 0;

    if(!d.failed<A2i>()){
        A2SData pre = d.success<A2i>();
        sum += pre.value;
        std::cout << "A2i " << pre.value << std::endl;
    }

    if(!d.failed<A3i>()){
        A3SData pre = d.success<A3i>();
        sum += pre.value;
        std::cout << "A3i " << pre.value << std::endl;
    }

    ctx.respond(boost::lexical_cast<std::string>(sum), "text/plain");

    std::cout << "Final end" << std::endl;
}).after(t2).after(t3);
\endcode

Once everything is set up we start the initial task `t1()`

\code
t1();
\endcode

> **note**
>
> By default if one subtask fails then all subtasks that depend on it will be cancelled and the final callback will be called immediately.
> :   However the sibling subtasks of the failing subtask will continue
>     executing. To change this behavior use `required(false)` on the
>     subtask.
>
> \code
> auto t1 = udho::activities::perform<A1>::with(data, io).required(false);
> \endcode
>
> In the above example all other subtasks will execute even if `t1`
> fails.