Execution of Asynchronous Tasks may be required for executing SQL or any other data queries including HTTP queries to fetch external data. udho does not including a database library with itself.
However any asynchronous database library using boost asio may benifit from an async activity framework. `udho::activities` provides an way to describe a task graph that gather data into an heterogenous collector.
A task may be associated with its dependencies of other tasks and task related data. 

An `activity` is the definition of the job that has to be performed. A `subtask` is an instantiation of an `activity`. Definition of an activity consists of two types `SuccessDataT`, `FailureDataT` and a no argument function call operarator `operator()()` overload.
A subtasks 
