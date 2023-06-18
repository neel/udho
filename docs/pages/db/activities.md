Pg Activity {#PgActivities}
===================

There can be various types of SQL queries.

Select
-------

@par SELECT'ing multiple or one row
`::fetch` is used when 0 or more rows are expected in the dataset.
`::retrieve` is used when 0 or one rows are expected in the dataset.
@code {.cpp}
using all_students = pg::from<students::table>
                       ::fetch
                       ::all
                       ::apply;
@endcode
@code {.sql}
select
    students.id,
    students.first_name,
    students.last_name,
    students.marks,
    students.age
from students
@endcode
Use `::by` to specify which field will be used to filter.
By default `::eq` (equals to) condition is used. (other operators are explained below)
@code {.cpp}
using one_student = pg::from<students::table>
                      ::retrieve
                      ::all
                      ::by<students::id>
                      ::apply;
@endcode
@code {.sql}
select
    students.id,
    students.first_name,
    students.last_name,
    students.marks,
    students.age
from students
    where
        students.id = $1
@endcode


@par WHERE queries
SQL queries intended to filter a subset of records use `WHERE` clause with a set of conditions that are composed
of a field an operator and a constant value. Following is an example usage of `NOT LIKE` operator.
@code {.cpp}
using search_students = pg::from<students::table>
                           ::fetch
                           ::all
                           ::by<students::first_name::not_like>
                           ::apply;
@endcode
@code {.sql}
select
    students.id,
    students.first_name,
    students.last_name,
    students.marks,
    students.age
from students
where
    students.first_name not like $1
@endcode
If the right hand side of the condition is another field then the operators `_` variants have to be used as shown in the following example.
@code {.cpp}
using special_students = pg::from<students::table>
                           ::retrieve
                           ::all
                           ::by<students::age::eq_<students::table::column<students::marks>>>
                           ::apply;
@endcode
@code {.sql}
select
    students.id,
    students.first_name,
    students.last_name,
    students.marks,
    students.age
from students
    where
        students.age = students.mark
@endcode
A list of possible operators are presented below
| C++          | SQL      | input    |
|--------------|----------|----------|
| `::eq`       | =        | constant |
| `::eq_`      | =        | field    |
| `::neq`      | !=       | constant |
| `::neq_`     | !=       | field    |
| `::lt`       | <        | constant |
| `::lt_`      | <        | field    |
| `::lte`      | <=       | constant |
| `::lte_`     | <=       | field    |
| `::gt`       | >        | constant |
| `::gt_`      | >        | field    |
| `::gte`      | >=       | constant |
| `::gte_`     | >=       | field    |
| `::like`     | LIKE     | constant |
| `::like_`    | LIKE     | field    |
| `::not_like` | NOT LIKE | constant |
| `::not_like_`| NOT LIKE | field    |
| `::is`       | IS       | constant |
| `::is_`      | IS       | field    |
| `::is_not`   | IS NOT   | constant |
| `::is_not_`  | IS NOT   | field    |
| `::in`       | IN       | constant |
| `::in_`      | IN       | field    |
| `::not_in`   | NOT IN   | constant |
| `::not_in_`  | NOT IN   | field    |
| `::is_null`  | IS null  |          |

@par ORDER LIMIT
@code {.cpp}
using all_students_top5 = pg::from<students::table>
                            ::fetch
                            ::all
                            ::descending<students::marks>
                            ::limit<5>
                            ::apply;
@endcode
@code {.sql}
select
    students.id,
    students.first_name,
    students.last_name,
    students.marks,
    students.age
from students
order by
    students.marks desc
limit $1 offset $2
@endcode
The default value of `limit` is set to `5` in this example. However it can be changed at runtime.
The offset value can also be set at rimtime.



@par Excluding and Including
A field can be excluded from the SQL query by using `::exclude`, while including new fields can be done through `::include`.
Two fields can be concatenated using @ref udho::db::pg::concat "pg::concat".
An alias can be created using `::as` or using @ref pg::alias.
@code {.cpp}
using fullname = pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>;
using all_students_name = pg::from<students::table>
                            ::fetch
                            ::all
                            ::exclude<students::first_name, students::last_name>
                            ::include<fullname::as<students::name>>
                            ::apply;
@endcode
@code {.sql}
select
    students.id,
    students.marks,
    students.age,
    concat(students.first_name,' ',students.last_name) as name
from students
@endcode

@par Aggregate functions
Like @ref udho::db::pg::concat "pg::concat" there are many other aggregate functions. Following is an
example of using unary function @ref udho::db::pg::avg "pg::avg"
@code {.cpp}
using students_project_avg = pg::from<students::table>
                                ::fetch
                                ::only<students::first_name, pg::avg<students::marks>>
                                ::group<students::first_name>
                                ::apply;
@endcode
It is probably not the best example of using `AVG()` but following is the SQL that the above mentioned
activity will generate.
@code {.sql}
select
    students.first_name,
    AVG(students.marks)
from students
group by students.first_name
@endcode
The example also demonstrate the usage of `group by` clause.
The `pg::concat` function has already been introduced in the previous example. Now the unary functions are listed below.
| C++                                 | SQL      | input    |
|-------------------------------------|----------|----------|
| @ref udho::db::pg::count "pg::count"| count    | field    |
| @ref udho::db::pg::min "pg::min"    | min      | field    |
| @ref udho::db::pg::max "pg::max"    | max      | field    |
| @ref udho::db::pg::avg "pg::avg"    | avg      | field    |
| @ref udho::db::pg::sum "pg::sum"    | sum      | field    |


Insert
-------

@par Plain INSERT query
@code {.cpp}
using student_add = pg::into<students::table>
                        ::insert
                        ::writables
                        ::apply;
@endcode
`::writables` skips the `readonly` columns specified using @ref udho::db::pg::readonly "pg::readonly" in the relation
@code {.sql}
insert into students
    (first_name, last_name, marks, age)
values
    ($1, $2, $3, $4)
@endcode

@par Returnning the newly inserted row
@code {.cpp}
using student_add = pg::into<students::table>
                        ::insert
                        ::writables
                        ::returning<students::id>
                        ::apply;
@endcode
returns the value of the `id` field (which is determined by the sequence) of the newly inserted row
@code {.sql}
insert into students
    (first_name, last_name, marks, age)
values
    ($1, $2, $3, $4)
returning id
@endcode
more than one field can be entered inside the returning clause
@code {.cpp}
using student_add = pg::into<students::table>
                        ::insert
                        ::all
                        ::returning<students::id, students::marks>
                        ::apply;
@endcode
`::all` specifies that not only writables, rather value of all fields will be provided
@code {.sql}
insert into students
    (id, first_name, last_name, marks, age)
values
    ($1, $2, $3, $4, $5)
returning id, marks
@endcode

Update
-------

@par Update a row
@code {.cpp}
using student_update = pg::into<students::table>
                         ::update
                         ::writables
                         ::by<students::id>
                         ::apply;
@endcode
@code {.sql}
update students
set first_name = $1,
    last_name  = $2,
    marks      = $3,
    age        = $4
where id       = $5
@endcode

@par Update all rows
@code {.cpp}
using student_update = pg::into<students::table>
                         ::update
                         ::all
                         ::apply;
@endcode
@code {.sql}
update students
set id         = $1,
    first_name = $2,
    last_name  = $3,
    marks      = $4,
    age        = $5
@endcode

Remove
-------

@code {.cpp}
using student_remove = pg::from<students::table>
                         ::remove
                         ::by<students::id>
                         ::apply;
@endcode
@code {.sql}
delete from students where id = $1
@endcode

Create
--------

@par Simple Create
@code {.cpp}
using create_articles_after_skip = pg::ddl<articles::table>
                                     ::create
                                     ::if_exists
                                     ::skip
                                     ::apply;
@endcode
@code {.sql}
create table if not exists articles(
    id bigserial primary key,
    title varchar unique,
    author bigint references students(id) on delete restrict,
    project bigint references projects(id) on delete restrict,
    published timestamp without time zone not null default now(),
    content text
)
@endcode

@par Excluding a field while creating
@code {.cpp}
using create_articles_except_after_skip = pg::ddl<articles::table>
                                            ::create
                                            ::except<articles::content>
                                            ::if_exists
                                            ::skip
                                            ::apply;
@endcode
@code {.sql}
create table if not exists articles(
    id bigserial primary key,
    title varchar unique,
    author bigint references students(id) on delete restrict,
    project bigint references projects(id) on delete restrict,
    published timestamp without time zone not null default now()
)
@endcode

@par Including only a subset of field while creating
@code {.cpp}
using create_articles_only_after_skip = pg::ddl<articles::table>
                                          ::create
                                          ::only<articles::id, articles::title, articles::author, articles::project, articles::published>
                                          ::if_exists
                                          ::skip
                                          ::apply;
@endcode
@code {.sql}
create table if not exists articles(
    id bigserial primary key,
    title varchar unique,
    author bigint references students(id) on delete restrict,
    project bigint references projects(id) on delete restrict,
    published timestamp without time zone not null default now()
)
@endcode

Drop
-----

@par Dropping one table
@code {.cpp}
using drop_students = pg::ddl<students::table>
                        ::drop
                        ::apply;
@endcode
@code {.sql}
drop table if exists students
@endcode
