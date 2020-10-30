
Async Task Graph
================

Execution of Asynchronous Tasks may be required for executing SQL or any other data queries including HTTP queries to fetch external data. udho does not including a database library with itself.
However any asynchronous database library using boost asio may benifit from an async activity framework. ``udho::activities`` provides an way to describe a task graph that gather data into an heterogenous collector.
A task may be associated with its dependencies of other tasks and task related data. 


Collector
---------

Different activities using different success and failure data will be involved in the subtask graph. A subtask or even an activity may need to use the data fetched by its dependencies in order to execute its action. Hence a heterogenous collector collects the data submited by the ``success`` or ``failure`` methods, which can be accessed through an `accessor`. In the following example ``data`` is a collector that collects success or failure data of three activities ``A1``, ``A2``, and ``A3`` respectively. Usually all activities involved in the subtask graph should be passed as a template parameter of ``collect``

.. code-block:: cpp

    auto data = udho::activities::collect<A1, A2, A3>(ctx, "Any string name for the collector");

However under many circumstances the entire collector is not required to be accessed. Instead a subset of what has been collected are accessed. So an ``accessor<...>`` is created from that collector.

.. code-block:: cpp

    udho::activities::accessor<A1, A2> access(data);
    if(!access.failed<A1>()){
        A1SData pre = access.success<A1>();
    }

The collector has to be passed to the constructor of all activities.

Activity
----------

An ``activity`` is the definition of the job that has to be performed. A ``subtask`` is an instantiation of an ``activity``. Definition of an activity consists of two types ``SuccessDataT``\ , ``FailureDataT`` and a no argument function call operarator ``operator()()`` overload. A ``subtask`` on the other hand instantiates an ``activity`` by specifying its data and execution dependencies thus establishing an order of invocation. The philosophy is that the activity is reusable while the subtask reuses that to model different use cases. The activity is invoked through its ``operator()()`` overload after all its prior activities have completed (dependencies are specified by the subtask). The ``operator()()`` must call either ``success()`` or ``failure()`` methods either synchronously or asynchronously with ``SuccessDataT`` or ``FailureDataT`` in order to signal its completion. An activity ``A`` subclasses from ``udho::activity<A, SuccessA, FailureA>`` which defines the ``success()`` and ``failure()`` methods.


In the following example the success and failure data for activity `A1` are ``A1SData`` and ``A1FData`` respectively. ``A1`` starts a timer for 5 seconds. Once the timer finishes the value 42 is set as the success value and the activity finishes.

.. code-block:: cpp

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

Subtask
-------

Activities serves the purpose of defining an action. however to action is supposed to fit in some use case depending on the application needs. The application needs may require a specific order of invocation of these activities. So that sufficient data is collected before some activity is invoked.

.. graphviz::

    digraph {
      A1 -> A2;
      A1 -> A3;
      A2 -> A4;
      A3 -> A4;
    }

In the above mentioned task graph, Both A2 and A3 depends on A1. Hence A2 and A3 may start as soon as A1 completes. A4 depends on both A2 and A3 and cannot start until both A2 and A3 completes. In this situation A2 and A3 can access the data fetched by A1 activity because that has already been completed (succeded or failed) before A2 or A3 is invoked. Similarly A4 can access the data collected by A1, A2, A3 that has completed before A4 has been invoked.

``usho::activities::perform`` describes the above subtask graph as shown in the following code block.

.. code-block:: cpp

    auto t1 = udho::activities::perform<A1>::with(data, io);
    auto t2 = udho::activities::perform<A2>::require<A1>::with(data, io).after(t1);
    auto t3 = udho::activities::perform<A3>::require<A1>::with(data, io).after(t1);
    auto t4 = udho::activities::perform<A4>::require<A2, A3>::with(data, io).after(t2).after(t3);

Example
#######

API
###

.. doxygenstruct:: udho::activities::activity
   :members:
   :inherited-members:

.. doxygenstruct:: udho::activities::subtask
   :members:

.. doxygenstruct:: udho::activities::perform
   :members:

.. doxygenstruct:: udho::activities::collector
   :members:

.. doxygenstruct:: udho::activities::accessor
   :members:
