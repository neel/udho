Layout {#LayoutPage}
==========================

```cpp
auto layout = udho::view::tmpl::layout::create<udho::view::tmpl::layout::placeholders::standard>(context);

layout.preamble().title("Page title");
layout.properties(placeholders::central).classes("central_class").id("central_id");

layout[placeholders::central] = "Hello";

layout();
```