***************
Request Context
***************

A context wraps a raw HTTP request and is passed to all callables once its corresponding url pattern is matched. The context also associates the request with the underlying server. 
So using the server the server common functions that are not specific any particular callable like logging, template rendering etc. can be performed. The context can be ``stateful`` or ``stateless``.
With a stateless context session informations can not be accessed even if the server is stateful. A stateful context is instantiated with a set of ``states`` that can be accessed from the callable.
The set of states with which the stateful context is created should be the subset of the states of a stateful server. As the stateful context contains a subset of server states only that subset of state information 
is accessible through the stateful context.

.. note:: The context templates require the bridge and HTTP request type to be passed as the template parameters while instantiation. 
          Its adviced to use the conveniance types instead of directly instiating these templates.
          
          .. code-block:: cpp
          
              str::string profile(udho::contexts::stateful<user> ctx){
                  if(ctx.session().exists<user>()){
                    user data;
                    ctx.session() >> data;
                    // user logged in
                  }else{
                    // user not logged in
                  }
              }
              
          :cpp:type:`udho::contexts::stateful` is actually a typedef of :cpp:type:`udho::context` with default bridge and request type
              
          .. code-block:: cpp
          
              std::string profile(udho::contexts::stateless ctx, std::size_t uid){
                  // view public profile of user uid
              }
              
        
          :cpp:type:`udho::contexts::stateless` is actually a typedef of :cpp:type:`udho::context< AuxT, RequestT, void >` with default bridge and request type


Request
#######

The request parameters can be accessed either through the conveniance methods or through the raw boost-beast request object. Use ``ctx.request()`` to get the underlying request object from the context.
However for common tasks there are conveniance methods that extracts information from the request object and returns.

.. doxygenfunction:: udho::context::request

.. doxygenfunction:: udho::context::target

.. doxygenfunction:: udho::context::path

.. doxygenfunction:: udho::context::query

Logging
#######

The context passes a logging message to the logger attached with the server.

.. doxygenfunction:: udho::context::log

API
###

          
Stateful Context Template
*************************

.. doxygenstruct:: udho::context
   :members:

Stateless Context Template
**************************
.. doxygenstruct:: udho::context< AuxT, RequestT, void >
   :members:
