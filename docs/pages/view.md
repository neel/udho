Views {#ViewsPage}
========

Views are the architectural component in the framework that formats and present the data. 
A C++ data object is provided to it as an input, along with some auxilary inputs such as a context.
A view extract the relevant parts of information from that data object and presents them in a suitable format.
It applies presentation logic on that data hierarchically which allows one view to embed another and thus implementing PAC (Presentation Abstraction Control).
Depending on the use case the views can have complicated presentation logic.
Generally views are written in a templating language, which is almost like a gloryfied string format.
But the expressibility and extendability offered by a templating language is very limited.
So, the framework allows views to be written in a foreign language to ensure that all possible presentation logics can be expressed.
Therefore the view utilizes the foreign langage bindings of the C++ data and the context object while the result of the view execution is taken back to C++.
However, this has its own trade offs as executing views written in another language may reduce performance.
The framework provides integration with lua, through sol2, which provvides good performance compared to many other alternatives.
Additionally there is a future plan to integrate with other languages such as javascript.
Moreover, if more performance is needed views can always be written in C++.
An example view written in Lua is shown below.

```html
<?! vars(d, ctx) require.js("profile.js"), require.css("profile.css") ?>

Hi <b><?= d.name ?></b>
Following are your addresses.
<ul>
<? for i, addr in d.addresses ?>
  <li> <b>(<?= i ?>):</b> <?= addr ?> </li>
<? end ?>
</ul>
```

The first line of the view specify the meta information of the view.
`vars` set the name of the variable that will be used inside the view body to refer to the data and the context object.
Here, in the above example we use `d` to refer to the data object passed to the view and `ctx` object to refer to the context.

Views can also embed another view using the context.
In the example below, the view embeds another view named address, intended to present address of the user.
We pass the `d.address` as data object to the child view.

```html
<?! vars('d', 'ctx') require.js("address.js") require.css("address.css") ?>

Embedding view
<?= ctx:view("address", "prefix"):render(d.address) ?>
```


## Store

The views are executed through a bridge.
There is a bridge for every supported foreign language. 
Currently there is only lua bridge as the framework only supports lua.
However it is possible to extend and support other languages too as mentioned above.
The assets generally do not need a bridge as they don't interact with C++ datastructure.
However both of these two types of resources are registered to the resource store as shown below.


```c++
static char buffer[] = R"TEMPLATE("<?! vars(d, ctx) ?>
...
)TEMPLATE";

namespace bridges   = udho::view::data::bridges;
namespace resources = udho::view::resources;

bridges::lua lua;
lua.init();

resources::store<bridges::lua> store{lua};
store["users"] << resources::lua("profile",     buffer_profile)
               << resources::lua("inbox",       buffer_inbox)
               << resources::js ("profile.js",  buffer_profile_js)
               << resources::css("profile.css", buffer_profile_css);

apps::some_application app{...};  // Instantiate the applications
app.init(store);                  // pass the store object to the applications 
                                  //   so that it can also register its views and assets
```

You may breakdown the webapplication into multiple applications or you may use user contributed apps.
However those apps also need to register their views and assets to the same store (may be under a different prefix).
So, the application may provide an init method that acceps the store as an argument.
Here in this example we consider an hypothetication application named `some_application` that has an init method.
However that class could also take that in the constructor. 
The framework does not impose any restruction on that. 

# Metatype

However, as we are using foreign language bindings to pass the C++ data object to the presentation abstraction control, we need to specify the bindings of the C++ type.
The bindings are specified using a metatype function as shown below.

```c++
struct person{
    std::string first_name;
    std::string last_name;
    double      age;
    address     permanent_address;

    friend auto metatype(udho::view::data::type<person>){
        using namespace udho::view::data;

        return assoc("person"),
            mvar("first_name",   &person::first_name),
            mvar("last_name",    &person::last_name),
            cvar("age",          &person::age),
            mvar("address",      &person::permanent_address);
    }
};
```

Inside the metatype function we state how this person object will be accessed from the foreign language.
Exposing a function with `cvar`/`mvar` functions allows them to be accessed as a property from the foreign language such as lua or javascript.
Following is a table of the functions used for exposing functions and member variables.

| Binder | Foreign             | C++             | Remarks |
|--------|---------------------|-----------------|---------|
| cvar   | immutable property  | member variable | C++ const variables may become mutable in lua due to issues in sol library. cvars can be transformed into json but cannot be loaded from json (because they are immutable)  |
| mvar   | mutable property    | member variable |         |
| fvar   | functional property | member variable | If a single member function provided then it represents a getter in the foreign language. If two function callbacks are provided then the second one should represent the setter.        |
| func   | function            | member function |         |
| index  | index access        | V at(Key)       | Bind operator[] or another equivalent function to provide index based access in the foreign language.  |
| iter   | iterator            | begin(), end()  | Bing the iterators to allow interation in the foreign language.        |
|        |                     |                 |         |

We have previously seen example usages of cvar and mvar binders.
In the following example we make the `at` function and the iterator availabe to the foreign language for index based access and for iteration.

```c++
struct education{
    using const_iterator = std::vector<specialization>::const_iterator;
    using size_type      = std::vector<specialization>::size_type;
    using value_type     = const specialization&;

    std::string course;
    std::string university;
    marksheet   marks;

    education() = default;
    education(const std::string c, const std::string& u): course(c), university(u) {}

    friend auto metatype(udho::view::data::type<education>){
        using namespace udho::view::data;

        return assoc("education"),
            mvar("course",      &education::course),
            mvar("university",  &education::university),
            cvar("marks",       &education::marks),
            iter(&education::begin, &education::end),
            index(&education::at, &education::size);
    }

    void add_specialization(const specialization& specialization){
        _specializations.emplace_back(specialization);
    }
    const_iterator begin() const  { return _specializations.begin(); }
    const_iterator end()   const  { return _specializations.end();   }
    size_type      size()  const  { return _specializations.size();  }
    value_type     at(std::size_t i) const {
        return _specializations.at(i);
    }

    private:
        std::vector<specialization> _specializations;
};
```
