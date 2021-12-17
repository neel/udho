/**
 * \defgroup udho
 * \brief udho HTTP framework
 */

/**
 * \defgroup hazo
 * \brief udho heterogenous data storage
 * \ingroup udho
 */

/**
 * @defgroup activities
 * @brief udho async activities.
 *
 * Execution of Asynchronous Tasks may be required for executing SQL or any other data 
 * queries including HTTP queries to fetch external data. Activities module provides a
 * generic framework of performing one or more asynchronous operations sequentially or 
 * parallelly using boost asio io context. All activities perform their corresponding 
 * operations asynchronously and signal its completion by storing a success or failure
 * result to a common heterogenous @ref udho::activities::collector "collector". The common 
 * heterogenous @ref udho::activities::collector "collector" is non-copiable and instantiated 
 * as shared pointer. The collector is usually accessed through a copiable full or partial 
 * @ref udho::activities::accessor "accessor" that provides R/W access to a subset of the main storage. 
 *
 * An activity is defined as a class that provides a no argument `operator()` overload and 
 * a pair of success `S` and failure `F` result types. An activity `X` inherits
 * from @ref udho::activities::activity "activity<X, S, F>" which provides two protected 
 * methods, @ref udho::activities::activity::success "success()" and \ref udho::activities::activity::failure "failure()". 
 * The `X::operator()` is supposed to start the asynchronous operation by specifying a callback 
 * function which will be called once the asynchronous operation completes. Depending on whether 
 * the operation is successful or not, the `finish` callback calls the `success()` or `failure()` 
 * methods of the base class with the corresponding result data to signal its completion.
 *
 * One activity may be dependent on completion of one or more other activities prior its 
 * execution. Hence a \ref udho::activities::combinator "combinator" is used to combine the 
 * dependencies of an activity `X` and on completion of all its dependencies, it calls or
 * cancels the invocation of `X::operator()`. e.g. if any of its required dependencies fail
 * then the activity `X` is canceled.
 * 
 * @image html activities-anatomy.png 
 * @ingroup udho
 */

/**
 * @defgroup db
 * @brief udho database functionalities
 * @ingroup udho
 */

 /**
  * @defgroup pg
  * @brief udho postgresql databae functionalities
  * @ingroup db
  */


#ifndef __DOXYGEN__

/**
 * \defgroup capsule
 * \brief Capsule serves as an encapsulation of the data inside the a node.
 * 
 * Some capsules may have a key_type which is used to store associative
 * data. Those capsules call the key() member of the underlying data
 * data structure. 
 * 
 * The type of the object encapsulated by the capsule is identified
 * as data_type as well as value_type. If the data_type a member function
 * named value() and a type named value_type then that type used as
 * the value_type of the capsule too.
 * \ingroup hazo
 */


/**
 * \defgroup encapsulate
 * \brief Helps encapsulation
 * \ingroup capsule
 */

/**
 * \defgroup node
 * \brief Constructs a chain of node which is used for hazo based sequence and map.
 * Each node contains a value of type HeadT and a tail.
 * The terminal node has void tail type.
 * A node internally uses `capsule` to store the data inside the node.
 * \code
 * node<A, node<B, node<C>, node<D, void>>>
 * \endcode
 * \tparam HeadT type of the value in a node
 * \tparam TailT type of the tail (void for terminal  node)
 * \ingroup hazo
 */


/**
 * \defgroup seq
 * \ingroup hazo
 */

/**
 * \defgroup map
 * \ingroup hazo
 */

#endif // __DOXYGEN__