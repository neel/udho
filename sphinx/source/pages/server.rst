HTTP Server
=============

udho provides varying types of servers depending on the logging and session facilities. A server is called stateful if it provides HTTP session, otherwise it is called stateless. If the server provides APIs that do noy require any session then use stateless server, otherwise use a stateful server. If no logging is required then use a ``quiet`` server that uses a void logger, otherwise use a logging server. The servers ``udho::servers::ostreamed::stateless`` and ``udho::servers::ostreamed::stateful<>`` used in the examples are stateless and stateful servers respectively that uses ostreamed logger present inside udho. All servers inside ``udho::servers`` namespace are typedef'ed ``udho::server``.

Logged Stateless Server
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   udho::servers::stateless::logged<LoggerT>
   udho::servers::logged<LoggerT>::stateless

Quiet Stateless Server
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   udho::servers::stateless::quiet
   udho::servers::quiet::stateless

Logged Stateful Server
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   udho::servers::stateful<States...>::logged<LoggerT>
   udho::servers::logged<LoggerT>::stateful<States...>

Quiet Stateful Server
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   udho::servers::stateful<States...>::quiet
   udho::servers::quiet::stateful<States...>

udho comes with a logger ``udho::loggers::ostream`` that prints logginf messages to ``std::ostream``. This logger is used in all examples. So there is a typedef ``servers::stateful<States...>::ostreamed`` that actually is ``servers::stateful<States...>::logged<loggers::ostream>``

A Logging server takes reference to a logger in constructor. So the Logger has to be instantiated by the caller, as shown in the following example.

.. code-block:: cpp

   udho::loggers::ostream logger(std::cout);
   udho::servers::stateful<StateA, StateB>::ostreamed server(logger);

For ease of access there is an ``ostreamed`` server is provided which owns the ostream logger inside. To use this server, no logger has to be instantiated. Only the stream (std::cout) has to be provided. through the constructor.

.. code-block:: cpp

   udho::servers::ostreamed::stateless
   udho::servers::ostreamed::stateful<StateA, StateB>

States
------

The stateful server uses session cookie for remembering a returning user. The session cookie is named ``UDHOSESSID``. For each session some information can be stored at server side. This type of features are useful for login, captcha, CSRF token etc.. udho takes advantage of stateful nation of C++ and provides session functionality around a set of states defined at compile time. A state can be any copiable C++ type. Once a stateful server is instantiated with ``N`` number of states, callables that takes stateful contexts with ``M <= N`` states can be used.

.. code-block:: cpp

   struct student{
       std::string  first_name;
       std::string  last_name;
       unsigned int registration_id;
   };
   struct appearence{
       std::string theme;
   };

Using these two states a server can be defined as following.

.. code-block:: cpp

   udho::servers::ostreamed::stateful<student, appearence> server(std::cout);

A function taking a stateful context around zero or more of these two states can be used in the router when the above server is used. So all four (\ ``func1``\ , ``func2``\ , ``func3``\ , and ``func4``\ ) are valid functions that can be mapped with the router when the above mentioned server is used.

.. code-block:: cpp

   std::string func1(udho::contexts::stateful<student, appearence> ctx){
       // ...
   }
   std::string func2(udho::contexts::stateful<student> ctx){
       // ...
   }
   std::string func3(udho::contexts::stateful<appearence> ctx){
       // ...
   }
   std::string func4(udho::contexts::stateless ctx){
       // ...
   }

In these functions the corresponding values of the current session of the requested type can be accessed like the following.

.. code-block:: cpp

   std::string func1(udho::contexts::stateful<student, appearence> ctx){
       bool student_data_exists = ctx.session().exists<student>();
       if(!student_data_exists){
           student data;
           data.first_name = "Jane";
           data.last_name  = "Doe";
           data.registration_id = 1;

           ctx.session() << data;
       }else{
           student data;
           ctx.session() >> data;
           std::cout << data.first_name << std::endl;
       }
   }
