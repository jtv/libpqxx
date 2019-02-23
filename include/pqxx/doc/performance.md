Performance features                       {#performance}
====================

If your program's database interaction is not as efficient as it needs to be,
the first place to look is usually the SQL you're executing.  But libpqxx
has a few specialized features to help you squeeze more performance out
of how you issue commands and retrieve data:

* @ref streams.  Use these as a faster way to transfer data between your
    code and the database.
* @ref prepared.  These can be executed many times without the server
    parsing and planning them each and every time.  They also save you having
    to escape string parameters.
* pqxx::pipeline lets you send queries to the database in batch, and
    continue other processing while they are executing.

As always of course, don't risk the quality of your code for optimizations
that you don't need!
