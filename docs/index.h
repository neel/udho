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