# Building the documentation

Documentation targets are available when Doxygen is installed, while Doxygen
remains optional for a normal library build. From the repository root,
configure the project and build the modular aggregate:

```sh
cmake -S . -B build
cmake --build build --target docs
```

Documentation is built only when one of its targets is requested explicitly;
it is never included in the default build. If Doxygen is unavailable,
configuration still succeeds but documentation targets are not created.

Each public module owns its Doxygen configuration at
`includes/udho/<module>/Doxyfile`. These files include the shared defaults from
`docs/Doxyfile` and can override any Doxygen feature independently. For
example, a module can enable Graphviz diagrams without adding corresponding
CMake options. A module configuration can also be run without CMake:

```sh
cd includes/udho/view
doxygen Doxyfile
```

Standalone output is written below the module's ignored `.doxygen/` directory.
The hand-written pages configuration can similarly be run from `docs/pages/`.
CMake generates only a small wrapper around these files to select build-tree
output and tag-file paths. The source-controlled `docs/Doxyfile.html` and
`docs/Doxyfile.tags` select the outputs for the two passes. Inputs, exclusions,
warnings, appearance, and all other Doxygen behavior remain in Doxyfiles.

Clang-assisted parsing uses the fixed compiler options in
`docs/compile_flags.txt`; it does not require generated compilation-database
entries or a documentation-only C++ target. Module Doxyfiles may append any
prerequisites needed by that module through `CLANG_OPTIONS`. Update the fixed
flags if the compiler or dependency layout changes.

The first pass generates XML and tag files for every module. The second pass
generates each module's HTML with all other module tags available, so
documentation-only cross-references and cyclic module relationships do not
require a manually maintained dependency graph. Tag targets are order-only
dependencies: an updated module tag is ready for a requested HTML build but
does not force unrelated module HTML to rebuild.

The modular output starts at `build/docs/modular/pages/html/index.html`. The
following targets are available:

* `docs-module-<name>` builds HTML and XML for one public module from
  `includes/udho/<name>`;
* `docs-pages` builds the hand-written pages without parsing public headers;
* `docs-modules` builds all module API sites;
* `docs-tags` builds cross-reference tag files and per-module XML;
* `docs` builds tags, module API sites, and pages in pass order.

Doxygen diagnostics are written to `doxygen-warnings.log` beside each modular
target's generated `html` and `xml` directories in the build tree.

Doxygen commands are backed by generated stamp files and explicit source
dependencies. Rebuilding an unchanged target is a no-op; changing a page only
updates `docs-pages`, while changing a header updates its module tag, XML, and
HTML. As with the library's existing header glob, add or remove a header by
rerunning the CMake configure step so it enters the dependency graph.
