*****************
Async Task Graph
*****************

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

    auto data = udho::activities::collect<A1, A2i, A3i>(ctx, "A");
    
    auto t1 = udho::activities::perform<A1>::with(data, io);
    auto t2 = udho::activities::perform<A2>::require<A1>::with(data, io).after(t1);
    auto t3 = udho::activities::perform<A3>::require<A1>::with(data, io).after(t1);
    auto t4 = udho::activities::perform<A4>::require<A2, A3>::with(data, io).after(t2).after(t3);

The above statements using ``perform`` are equivalent to the following.

.. code-block:: cpp

    auto data = udho::activities::collect<A1, A2i, A3i>(ctx, "A");

    auto t1 = udho::activities::subtask<A1>::with(data, io);
    auto t2 = udho::activities::subtask<A2, A1>::with(data, io).after(t1);
    auto t3 = udho::activities::subtask<A3, A1>::with(data, io).after(t1);
    auto t4 = udho::activities::subtask<A4, A2, A3>::with(data, io).after(t2).after(t3);
    
With this arrangement Success/Failure data of A1 is available to A2 and A3 before it starts execution. To access that data the constructors on A2 and A3 may create an accessor as shown below.

.. code-block:: cpp

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
    
However in the above mentioned implementation A2 always depends on A1. In an use case where A2 depends on some other activity or even executed independently won't be feasible because A2 will still have an accessor to A1 and it will try to extract the success data of A1 irrespective of the graph. On the other hand, A2 may still need some data that has to be set in order to perform the activity. that data may be derived from A1 or from some other activity or through some constructor argument.

.. code-block:: cpp

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
    
In the above example ``prevalue`` is that piece of information that is required by ``A2i`` in order to perform its activity. It doesn't care who provides that information, whether its A1, or some other activity. ``A2i`` has a constructor that takes that ``prevalue`` as an argument which can be used when ``prevalue`` is known at the time of construction, or when ``A2i`` is the starting activity in the graph. 

.. code-block:: cpp

    auto t2 = udho::activities::perform<A2i>::require<A1>::with(data, io).after(t1).prepare([data](A2i& a2i){
        udho::activities::accessor<A1> a1_access(data);
        A1SData pre = a1_access.success<A1>();
        a2i.prevalue = pre.value;
    });
    
In the above code block that activity ``A2`` is prepared through its reference passed as the argument right after all its dependencies (only A1 in this case) completes by the callback provided in the ``prepare`` function. The collected ``data`` is captured inside the lambda function from which the ``A1`` success data is accessed.
    
Final
-----

Finally we would like do something once the entire subtask graph has completed in a similar fasion. In the following example we do not perform ``A4``. Instead we execute the following piece of code once both ``A2i`` and ``A3i`` completes.

.. code-block:: cpp

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
    
Once everything is set up we start the initial task ``t1()``

.. code-block:: cpp

    t1();
    
.. note:: By default if one subtask fails then all subtasks that depend on it will be cancelled and the final callback will be called immediately.
          However the sibling subtasks of the failing subtask will continue executing. To change this behavior use ``required(false)`` on the subtask.
          
          .. code-block:: cpp

              auto t1 = udho::activities::perform<A1>::with(data, io).required(false);
              
          In the above example all other subtasks will execute even if ``t1`` fails.
    
Example
-------

API
---

Data
****

.. doxygenstruct:: udho::activities::collector
   :members:

.. doxygenstruct:: udho::activities::accessor
   :members:
   
Activity 
********

.. doxygenstruct:: udho::activities::activity
   :members:

.. doxygenstruct:: udho::activities::result
   :members:
   
.. doxygenstruct:: udho::activities::result_data
   :members:
   
Subtask
*******

.. doxygenstruct:: udho::activities::subtask
   :members:

.. doxygenstruct:: udho::activities::perform
   :members:

