Layout {#LayoutPage}
==========================

```cpp
auto layout = udho::view::tmpl::layout::create<udho::view::tmpl::layout::placeholders::standard>(context);

layout.preamble().title("Page title");
layout.properties(placeholders::central).classes("central_class").id("central_id");

layout[placeholders::central] = "Hello";

layout();
```

Use `attach` to retain the layout document until the response completes.

```cpp
namespace layout = udho::view::tmpl::layout;

auto page = layout::attach<layout::placeholders::standard>(context);
auto custom_page = layout::attach<MyPlaceholders, MyPresenter>(context);
```

