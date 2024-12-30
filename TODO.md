const_tore is exposed to lua in two ways.
    1. through bind specialization in lua.h
    2. through metatype
    
The bind specialization aims to utilize the metatype based automatic binding first, and then it adds lua specific view function.

> DONE TODO: It is due to be checked that this dual specification of bindings work or not.

The work has been started in sandbox.cpp where we pass the const_store to the view. 
However the existing lua view code tests the routes which was part of the previous work on exporting the router summary.
Now we need probably need to add a new view to check the resources.

> DONE TODO: Add a new lua view to check const_store.

Then try to embed one view inside another.

> DONE TODO: Test view embedding.
> But needs more work to organize things in a better way.

> DONE TODO: Specifying <?! vars('d') ?> did not work but <?! vars('d', 'ctx') ?> works.

In order to actually test this kind of things we really need a testing context which is not created by the http server upon receiving an incoming connection.
We can use that for unit testing.

> DONE TODO: Create fake context.

> DONE TODO: pronbably there is a problem with fvar handling getters returning const reference.
> Some types were not bounded

> DONE TODO: in sandbox.cpp udho::url::summary::mount_point::url_proxy type is not probably being binded automatically. Check that.
> Fixed. Should work now.

> DONE TODO: Automatic type binding not working always.

> DONE TODO: view key is calculated as udho::url::format(":{}/{}", self.prefix(), self.name()) but there are two places where view key is calculated. Make single entry.

> TODO: Need to document the tmpls, asset etc.. Clearly state what are movable, what are copiable etc..

> DONE TODO: Thee two types of resources have to be dealth in a different way. The view can only be a file or memory, whereas an asset can be file, memory, url. Also, the asset store doesn't actually store the assets.

> TODO: layout

> TODO: asset store need to use iterator adopters around the unique_ptr<abatract_resource>

> TODO: asset store need to integrate with the mount_point to provide functions like url(). It should also provide functions such as contents(), base64() etc..

> TODO: router needs to have two functionalities
>       1. access to asset store (udho::view::resources::asset::const_store)
>       2. list of filesystem paths from where to serve static files 

> TODO: Formalize an way of throwing http exception from the actions or other ways of commucating http errors.

> TODO: Router serving assets if it could not files with matching url pattern in the document root

> DONE TODO: How do I add mime type ?
> mime method added to the basic_resource class

> TODO: Sometimes I need the view to clean white spaces. So that <?= 'a' ?> woull not be precedded by while spaces because of the indentation in view

> TODO: lua bridge index mutable index is still pending See udho/view/data/nvp.h
> Defered because views don't alter data
