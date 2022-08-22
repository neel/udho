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
  * Schema
  * -------
  * 
  * A schema consists of multiple field or *field like* objects. It is used to define the fields 
  * in a relation or a record received from an SELECT query. It is also used as a type-safe heterogenous 
  * container, for INSERT, and UPDATE queries. It is also used to set values of the fields in the 
  * WHERE clause.
  * 
  * A schema is defined using @ref udho::db::pg::schema "pg::schema<X...>" where the each of 
  * these X are fields or *field likes* e.g. @ref udho::db::pg::column "pg::column<Field, Relation>".
  *
  * Given the above mentioned relation `table` we can have `table::all` as a schema that includes all the fields 
  * in the relation. Once we have a schema we can generate a new schema that includes or excludes one or more fields. 
  * `table::all::exclude<last_name>` is same as `pg::schema<id, first_name>`. 
  *
  * Query
  * ------
  * 
  * The select queries start from @ref udho::db::pg::from "pg::from<RelationT>" where the `RelationT` is the struct 
  * that defines the relation. In the following example `all` represents a `SELECT * from users` query. 
  *
  * @code 
  * using all = pg::from<table>
  *    ::fetch
  *    ::all
  *    ::apply;
  * @endcode
  * 
  * Connection
  * -----------
  * 
  * A connection pool has to be created using ozo
  * 
  * @code {.cpp}
  * ozo::connection_pool_config dbconfig;
  * const auto oid_map = ozo::register_types<>();
  * ozo::connection_info<> conn_info("host=localhost dbname=DBNAME user=USERNAME password=PASSWORD");
  * auto pool = ozo::connection_pool(conn_info, dbconfig);
  * @endcode
  *
  * Activity 
  * --------- 
  * 
  * @code {.cpp}
  * auto start = udho::db::pg::start<all>::with(ctx, _pool);
  * auto fetch_all = pg::after(start).perform<all>(start);
  * pg::after(fetch_subscribers).finish(start, [ctx](const pg::data<all>& d) mutable{
  *   auto users = d.success<all>();
  *   for(const auto& user: users){
  *     int id                 = user[id];
  *     std::string first_name = user[first_name];
  *     std::string last_name  = user[last_name];
  *   }
  * });
  * @endcode
  * 
  * 
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

 /**
  * @defgroup joining
  * @brief Describing a JOIN clause.
  * 
  * JOIN clauses are described mainly using three templates, \ref udho::db::pg::attached, \ref udho::db::pg::basic_join and 
  * \ref udho::db::pg::basic_join_on. At the lowest level one or more JOIN clauses are described by \ref udho::db::pg::basic_join_on. 
  * However, \ref udho::db::pg::basic_join is wraps it to make the code more readable. Finally \ref udho::db::pg::attached makes it 
  * little more easier to start with.
  * 
  * However it is not necessary to start with \ref udho::db::pg::attached either. The query building generally starts with `pg::from` 
  * that provides a join typedef which uses `pg::attached` internally.
  * 
  * @code 
  * pg::from<articles::table>                            // FROM table (0)
  *   ::join<students::table>                            // JOIN table (1)
  *     ::inner::on<articles::author, students::id>      // lhs (0), rhs (1)
  * @endcode 
  * @note in the documentation (0) refers to the relation used in the FROM clause and the fields associated with it. 
  *       Any integer n > 0 is used to refer the same for the n'th relation present in the subsequent JOIN clauses.
  * 
  * The above can be used to generate the following SQL.
  * @code 
  * select                              
  *     articles.id,                    
  *     articles.title,                 
  *     articles.author,                
  *     articles.project,               
  *     students.id,                    
  *     students.name,                  
  *     students.project,               
  *     students.marks                  
  * from articles                                // Table (0)
  * inner join students                          // Table (1)
  *     on articles.author = students.id         // lhs (0), rhs (1)
  * @endcode 
  * Following is the example of joining two tables with table (0)
  * @code 
  * pg::from<articles::table>                                   // FROM table (0)
  * ::join<projects::table>                                     // JOIN table (1)
  *   ::inner::on<articles::project, projects::id>              // lhs (0), rhs (1)
  * ::join<students::table>                                     // JOIN table (2) 
  *   ::inner::on<articles::author, students::id>               // lhs (0), rhs (2)
  * @endcode 
  * The above code may be used to generate the following SQL
  * @code 
  * select                                
  *     articles.id,                      
  *     articles.title,                   
  *     articles.author,                  
  *     articles.project,                 
  *     projects.id,                      
  *     projects.title,                   
  *     projects.admin,                   
  *     students.id,                      
  *     students.name,                    
  *     students.project,                 
  *     students.marks                    
  * from articles                            // FROM table
  * inner join projects                      // JOIN table (1)
  *     on articles.project = projects.id    // JOIN       (1)
  * inner join students                      // JOIN table (2)
  *     on articles.author = students.id     // JOIN       (2)
  * @endcode 
  * In the above example both projects and  students table is joined with `articles` table (which is in the FROM). So it was  
  * not reqired to specify the `articles` relation again. However when JOIN relates to some relation which is not in the FROM
  * clause, then it is necessary to specify that relation.
  * @code 
  * pg::from<articles::table>                                   // FROM table (0)
  * ::join<projects::table>                                     // JOIN table (1)
  *   ::inner::on<articles::project, projects::id>              // lhs (0), rhs (1)
  * ::join<students::table, projects::table>                    // JOIN table (2), JOIN table (1)
  *   ::inner::on<projects::admin, students::id>                // lhs (1), rhs (2)
  * @endcode 
  * The above can be used to generate the following SQL
  * @code 
  * select                                
  *     articles.id,                      
  *     articles.title,                   
  *     articles.author,                  
  *     articles.project,                 
  *     projects.id,                      
  *     projects.title,                   
  *     projects.admin,                   
  *     students.id,                      
  *     students.name,                    
  *     students.project,                 
  *     students.marks                    
  * from articles                            // FROM table     
  * inner join projects                      // JOIN table (1)
  *     on articles.project = projects.id    // JOIN       (1)
  * inner join students                      // JOIN table (2)
  *     on projects.admin = students.id      // JOIN       (2)
  * @endcode 
  * Another example
  * @code 
  * pg::from<articles::table>                                   // FROM table (0)
  * ::join<students::table>                                     // JOIN table (1)
  *   ::inner::on<articles::author, students::id>               // lhs (0), rhs (1)
  * ::join<memberships::table, students::table>                 // JOIN table (2), JOIN table (1)
  *   ::inner::on<students::id, memberships::student>           // lhs (1), rhs (2)
  * ::join<projects::table, memberships::table>                 // JOIN table (3), JOIN table (2)
  *   ::inner::on<memberships::project, projects::id>           // lhs (2), rhs (3)
  * @endcode 
  * The above may be used to generate the following SQL 
  * @code 
  * select                                   
  *     articles.id,                         
  *     articles.title,                      
  *     articles.author,                     
  *     articles.project,                    
  *     students.id,                         
  *     students.name,                       
  *     students.project,                    
  *     students.marks,                      
  *     memberships.id,                      
  *     memberships.student,                 
  *     memberships.project,                 
  *     projects.id,                         
  *     projects.title,                      
  *     projects.admin                       
  * from articles                            
  * inner join students                      
  *     on articles.author = students.id     
  * inner join memberships                   
  *     on students.id = memberships.student 
  * inner join projects                      
  *     on memberships.project = projects.id 
  * @endcode 
  * 
  * @ingroup crud
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
