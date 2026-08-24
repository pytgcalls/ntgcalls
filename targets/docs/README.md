# Documentation target

Renders the NTgCalls reference pages for <https://pytgcalls.github.io> from
`schema.ntl`, the same file the language bindings are generated from, so a
signature can never drift from the code it documents.

## Generating

```
cmake -DROOT_DIR=<repo root> -P cmake/codegen/GenerateDocs.cmake
```

Pages land in `docs-output/NTgCalls/<Section>/<Title>.xml`, mirroring the
layout of the `pytgcalls/docsdata` repository. One page carries every
language: the example block is a `<multisyntax id="languages">` tab set and
the details block is one `<lang-block language="...">` per language, all
driven by the reader's language choice, which the site stores globally.

## What lives where

| Content | Owner |
| --- | --- |
| Signatures, parameters, types, enum members, struct fields | `schema.ntl`, generated |
| Section and page title | `pages.cfg` |
| Exceptions listed per method | `pages.cfg`, `[raises]` |
| Prose key overrides | `prose.cfg` |
| Every line of prose, every example, every highlight range | `docsdata/config.xml` |

Examples are generated too: each language template emits a call skeleton
built from the parameter list, so every method has a runnable-shaped example
in all five languages. A template marks where the call starts with a bare
`@@mark@@` line; the driver strips it, counts the lines and turns it into the
`mark` attribute, so the highlighted range follows the example instead of
being maintained by hand. An `X_EXAMPLE_<LANG>` entry in `config.xml`
overrides the generated example for that page and language, and then its
`X_EXAMPLE_<LANG>_MARK` supplies the range.

A page never contains prose. It references it by id, derived from the page
title: `Connect P2P` gives `CONNECT_P2P_DESC` for the summary,
`CONNECT_P2P_DESC2` for the details body and `CONNECT_P2P_EXAMPLE_PYTHON`
for the Python example, and `CONNECT_P2P_EXAMPLE_PYTHON_MARK` for the lines
that example highlights. Parameters and struct fields use the field name, so
`user_id` gives `USER_ID_DESC` and the text is shared by every page that takes
that parameter. When a name means different things on different methods,
`prose.cfg` overrides it per method under `[scoped]`.

## Checking coverage

```
cmake -DDOCS_OUT=<repo root>/docs-output -DCONFIG_XML=<docsdata>/config.xml \
      -P cmake/codegen/CheckDocsCoverage.cmake
```

Fails with the list of ids the pages ask for and `config.xml` does not
define. A method absent from `pages.cfg` is reported as a warning by the
generator and gets no page.

## Publishing

`.github/workflows/docs.yml` regenerates the pages on every change to the
exposed headers or to this target, checks coverage, replaces the reference
pages in `pytgcalls/docsdata` and splices the sidebar groups between the
`GENERATED INDEX` markers in its `config.xml`. It needs a `DOCS_TOKEN`
`DOCS_DEPLOY_KEY` secret holding the private half of a deploy key with write
access to that repository. The target branch follows the
branch being pushed: `dev` publishes the beta reference to `docsdata` `dev`,
`master` publishes the stable one to `docsdata` `master`. The input overrides
that when the workflow is run by hand.

## The library version

`config.xml` states the library version in its `NTGCALLS_VERSION` option, and
the publish workflow writes it there from the version the build itself
reports, the same way `package.json` and `Cargo.toml` are stamped:

```
cmake -DCONFIG_XML=<docsdata>/config.xml -DOPTION=NTGCALLS_VERSION \
      -DVERSION=$(python setup.py --version) \
      -P cmake/codegen/StampDocsVersion.cmake
```

The `+dev.<sha>` suffix a local build carries is dropped. `PYTGCALLS_VERSION`
belongs to the other repository and is not touched here.

## Adding a language

Copy one of the numbered templates, change the type map and the naming
filter, add the language to `DOCS_LANGUAGES` in `GenerateDocs.cmake`, then add
a `<tab>`, a `<syntax-highlight>` and an `@insert` line to `90-page.tpl`.
Python, C, Node.js, Java and Rust are generated today.
