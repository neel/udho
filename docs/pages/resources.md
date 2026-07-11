Resources {#ResourcesPage}
=============

There are mainly two types of resources that we deal with in this framework. 
One is asset and the other one is view.
The resurces can be loaded from on memory buffer as well as from disk.
The resource store is responsible for storage, retrival and execution (for views) of resources.

@image html resources.png width=80%

## Assets

Assets include static resources like JavaScript files, stylesheets, and images. 
They are served directly via HTTP requests and can be processed server-side (e.g., compression and bundling) but do not interact directly with C++ data structures.

## Views
Views are the architectural component in the framework that formats and present the data. 
A C++ data object is provided to it as an input, along with some auxilary inputs such as a context.
A view extracts the relevant parts of information from that data object and presents them in a suitable format.
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

### Bridge
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
store["users"] << resources::lua("profile",     buffer_profile)             // lua view
               << resources::lua("inbox",       buffer_inbox)               // lua view
               << resources::js ("profile.js",  buffer_profile_js)          // js 
               << resources::css("profile.css", buffer_profile_css);        // css
               
store.assets().base("assets");                                              // docroot
store.lock();                                                               // lock store

const_store<bridges::lua> cstore{store};                                    // use const_store for access
```
