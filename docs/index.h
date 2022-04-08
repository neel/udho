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
 * @copydoc Activities
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

/**
  * @defgroup schema
  * @brief Definitions of structural constructs like field, column, relation etc...
  * 
  * The schema submodule in db.pg module is used for defining the structural constructs like
  * the field, relation, column etc... 
  * 
  * Field
  * -----
  * 
  * A field has a name and is associated with a PostgreSQL data type. The postgresql type has
  * a C++ type associated with it. Get and Set operations on that field expects the value operand
  * to be of that C++ type. The fields are created using the @ref PG_ELEMENT macro, which takes 
  * the name of the field and the PostgreSQL type as inputs.
  *  @code 
  *  PG_ELEMENT(id,          pg::types::integer);
  *  PG_ELEMENT(first_name,  pg::types::varchar);
  *  PG_ELEMENT(last_name,   pg::types::varchar);
  *  @endcode 
  * The PostgreSQL database offer type casting facility. An SQL query may associate the same
  * field with different types using [CAST](https://www.postgresql.org/docs/9.2/typeconv-oper.html)
  * operator. Therefore the macro `PG_ELEMENT` creates a template `Name_<T>` and instantiates
  * `Name_<Type>` as typedef `Name`. For example in the above mentioned example of `first_name`
  * field, the `PG_ELEMENT` macro will create a template `first_name_<T>` where `T` can be any 
  * PostgreSQL datatype. It also instantiates that template with the provided `Type` which is
  * `pg::types::varchar` as shown below.
  * @code
  * typedef first_name_<pg::types::varchar> first_name;
  * @endcode
  * 
  * All field and *field like* constructs have a `key()` and a `ozo_name()` method that returns 
  * name of the field as compile time strings (boos::hana::string and OZO string respectively).
  * 
  * Schema
  * -------
  * 
  * A schema consists of multiple field or *field like* objects. It is used to define the fields 
  * in a relation or a record received from an SELECT query. It is also used as a type-safe heterogenous 
  * container, for INSERT, and UPDATE queries. It is also used to set values of the fields in the 
  * WHERE clause.
  * 
  * A schema is defined using @ref udho::db::pg::schema "pg::schema<X...>" where the each of 
  * these X are fields or *field likes*. 
  * 
  * A schema is decorated using @ref decorators.
  * 
  * Parts of SQL queries against a schema can be generated using multiple @ref generators.
  * 
  * Relation
  * ---------
  * 
  * A relation specifies a set of fields that its consists of and it has a name e.g. a table or a 
  * view, A relation `X` subclasses from the @ref udho::db::pg::relation "pg::relation<X, Fields...>",
  * where `Fields...` are the fields inside that relation.
  * @code 
  * struct table: pg::relation<table, id, first_name, last_name>{
  *     PG_NAME(users)
  *     using readonly = pg::readonly<id>;
  * };
  * @endcode 
  * 
  * Parts of SQL queries for a relation can be generated using multiple @ref generators.
  * 
  * Column
  * --------
  * 
  * A column is created using the @ref udho::db::pg::column "pg::column<F, R>" macro where the F is a
  * field and R is a relation. In simple words, a column binds a field with a relation. 
  * 
  * @ingroup pg
  */

/**
  * @defgroup decorators
  * @brief udho postgresql schema decorators.
  * Decorates a schema and generate OZO string with placeholders and a tuple of parameters.
  * 
  * @ingroup pg
  */

/**
  * @defgroup generators
  * @brief udho postgresql query generator
  * @ingroup pg
  */

/**
  * @defgroup crud
  * @brief udho postgresql query model
  * @ingroup pg
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