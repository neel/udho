Pg Subtasks {#PgSubtasks}
=====================

A `pg::activity` encapsulates a prepared statement and associates a schema of expected resultset.
However, the business logic of the application may need multiple of such activities to be executed
in a specified order.
In udho we instantiate these activities as a `subtask` which specifies the order of evaluation
by specifying the previous or the next activities.
It also allows the results from all activities to be collected in one `collector`.
Following is an example.
We mention all activities that we will be executing in any order.
It is not required to be executed in the same order it is mentioned in the `pg::start`.

@code {.cpp}
auto start = pg::start<
    articles::drop,         // DROP table articles
    students::drop,         // DROP table students
    articles::create,       // CREATE table articles
    students::create,       // CREATE table students
    students::insert,       // INSERT one student
    articles::insert,       // INSERT one article
    students::all,          // SELECT all students
    students::like,         // SELECT students with similar first name
    students::by_id,        // SELECT student by id
    articles::all           // SELECT all articles
>::with(ctx, pool);
@endcode

@note The activities are defined in the @ref PgSubtasks_activities "Appendix"

@par If one activity fails then all activities that depend on it will also be cancelled.

The `start` mentioned above creates a collector and an initial activity works as an empty subtask but
doesn't perform any SQL query.
Now we can create other subtasks that depend on the `start` subtask to finish.
`start` subtask to finishes immediately as it doesn't perform any SQL query.
In the example below we specify that the subtask `drop_articles` will be executed after `start`.
Similarly `drop_students` will be executed after `drop_articles` has finished.

@code {.cpp}
auto drop_articles   = pg::after(start).perform<articles::drop>(start);
auto drop_students   = pg::after(drop_articles).perform<students::drop>(start);
@endcode

We wait for both drop subtasks to finish before creating the students table.
`create_articles` depends on `create_students` because the create statement contains foreign key references.

@code {.cpp}
auto create_students = pg::after(drop_articles, drop_students).perform<students::create>(start);
auto create_articles = pg::after(create_students).perform<articles::create>(start);
@endcode

Once all these tables are created we insert records into these tables.

@code {.cpp}
auto insert_student  = pg::after(create_students).perform<students::insert>(start, pg::types::varchar::val("Neel"), pg::types::varchar::val("Basu"));
@endcode

We may provide the values in the perform function but that is not necessary.

@par Alternatively we may instantiate the subtask without any argument as shown in the following example
@code {.cpp}
auto insert_student  = pg::after(create_students).perform<students::insert>(start);
insert_student[students::first_name::val] = pg::oz::varchar("Neel");
insert_student[students::last_name::val]  = pg::oz::varchar("Basu");
@endcode

After the student record is create we create a article

@code {.cpp}
auto insert_article  = pg::after(insert_student).perform<articles::insert>(start, pg::types::varchar::val("Article Title"), 1);
@endcode

Then we make the select queries

@code {.cpp}
auto fetch_students  = pg::after(create_articles, create_students).perform<students::all>(start);
auto students_like   = pg::after(create_articles, create_students).perform<students::like>(start);
auto find_student    = pg::after(create_articles, create_students).perform<students::by_id>(start);
auto fetch_articles  = pg::after(create_article).perform<articles::all>(start);

students_like[students::id::gte::val] = 2;
students_like[students::fullname::like::val] = std::string("Hello");

find_student[students::id::val] = 1;
@endcode

Once all these subtasks are finished we need process the results.

@code {.cpp}
pg::after(fetch_students, students_like, fetch_articles, find_student)
    .finish(start, [&](const pg::data<students::all, articles::all, students::insert>& d){
        // We need result from a subset of activities
        // If we need more then we can put those activities into pg::data<...> template
        // We can check this `d` for success, failure.
        // We can also extract the results from `d` as shown below.

        if(d.failed<students::all>()){
            const auto& failure = d.failure<students::all>();
            std::cout << failure.reason << std::endl;
        }else{
            const auto& results = d.success<students::all>();
            nlohmann::json json = results;
            std::cout << json << std::endl;
            std::cout << results.count() << " records" << std::endl;
            for(const auto& result: results){
                result[students::id::val];
                std::cout << result[students::id::val] << " " << result[students::first_name::text::val] << " " << result[students::last_name::text::val] << std::endl;
            }
        }

    });
@endcode

Finally we invoke the `start` which ends up executing the callback provided in `finish` after
the tree of subtasks end.

@code {.cpp}
start();
@endcode

Appendix Activities {#PgSubtasks_activities}
--------------------

@code {.cpp}

namespace students{

using drop     = pg::ddl<students::table>
                   ::drop
                   ::apply;

using create  = pg::ddl<students::table>
                   ::create
                   ::if_exists
                   ::skip
                   ::apply;

using all      = pg::from<students::table>
                   ::fetch
                   ::all
                   ::exclude<students::first_name, students::last_name>
                   ::include<students::first_name::text, students::last_name::text>
                   ::include<pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>::as<students::name>>
                   ::apply;

using by_id    = pg::from<students::table>
                   ::retrieve
                   ::all
                   ::exclude<students::first_name, students::last_name>
                   ::include<students::first_name::text, students::last_name::text>
                   ::include<pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>::as<students::name>>
                   ::by<students::id>
                   ::apply;

PG_ERROR_CODE(by_id, boost::beast::http::status::forbidden)

using fullname = pg::concat<students::first_name, pg::constants::quoted::space, students::last_name>;
using like     = pg::from<students::table>
                   ::fetch
                   ::all
                   ::include<fullname::as<students::name>>
                   ::by<students::id::gte, fullname::like>
                   ::apply;

using insert   = pg::into<students::table>
                   ::insert
                   ::only<students::first_name, students::last_name>
                   ::returning<students::id>
                   ::apply;

}

namespace articles{

using drop   = pg::ddl<articles::table>
                 ::drop
                 ::apply;

using create = pg::ddl<articles::table>
                 ::create
                 ::if_exists
                 ::skip
                 ::apply;

using all    = pg::from<students::table>
                 ::join<articles::table>::inner::on<students::id, articles::author>
                 ::fetch
                 ::only<articles::table::all>
                 ::exclude<articles::created, articles::title>
                 ::include<articles::title::text>
                 ::include<students::id::as<students::writer>>
                 ::include<students::first_name::text, students::last_name::text>
                 ::apply;

using insert = pg::into<articles::table>
                 ::insert
                 ::only<articles::title, articles::author>
                 ::returning<articles::id>
                 ::apply;

}

@endcode
