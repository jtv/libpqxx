Threat Model
============

Libpqxx is a client library for PostgreSQL.  It's a wrapper for libpq.

It assumes that it's talking to a genuine and fully patched libpq, which in
turn connects to a genuine and fully patched PostgreSQL server.  It also
assumes that the application calling it is not crafted to fool libpqxx, but
that inputs or data stored inside the database may be.  Environment variables
are assumed to be set by trusted users.

So the security concerns are with data passed to libpqxx functions by the
application, or retrieved from a database and returned to libpqxx from libpq.


Prioritisation
--------------

Stored credentials are a High priority (unless they're meant to be public
knowledge anyway; or if they are clearly local, transient, and safe from
hostile exploitation).

The tests in the `test/` directory may do strange things, because they're
_meant_ to exercise the edges and the dark corners.  Memory bugs, undefined
behaviour etc. in tests have Low priority.  Otherwise, report security
vulnerabilities in the tests only if they can be exploited by an attacker
without privileged access to the system running the tests.

Vulnerabilities or bugs that can only be exercised through deprecated features
(whether in libpqxx or a dependency) have reduced priority.


Style guide
-----------

Keep your code, documentation, and other text to 79 columns.  Capitalise and
punctuate code comments just like you would the same text in documentation.

In Markdown text, write identifiers (e.g. C++ classes or Python variables) in
backquotes (`transaction_base` etc.).  Not just for typesetting but also to
set them apart from common words.

Always use spaces for indentation, never tabs, _except:_

* where the `make` format requires tabs (including Maketile templates), or
* where we conventionally use them in m4, or
* in scripts related to GNU autotools: `configure`, `config/compile`, etc.

Try to ensure that Markdown files pass a `markdownlint` check:

* Start with a top-level header.
* Avoid mixing Markdown header styles.
* Separate each list from its preceding paragraph with a blank line.
* Write nontrivial code blocks between triple backquotes; specify language.
* Write filenames in backquotes to avoid trouble with un-escaped underscores.

When stating conditions in the Impact section of a report ("This needs", "Two
things most hold", etc.), make sure that the _purpose_ is not ambiguous: are
those the conditions for reproducing the problem, or for fixing it?

Feel free to capitalise the "l" in "libpqxx" at the beginning of a sentence.

I like to pronounce SQL as "S. Q. L."  So if you need an indefinite article
directly in front of it, please make it "an", not "a".

Oh, and "prepend" is not a word.  You probably mean "prefix."


Python formatting
-----------------

Follow the style guide as given above.  In addition:

* Indent by 4 spaces.
* Do not horizontally align arguments with the first one; just 4 spaces.
* In multi-line lists and tuples, keep a comma at the end for regularity.
* When line-breaking a function call, break the line right after the `(`.
