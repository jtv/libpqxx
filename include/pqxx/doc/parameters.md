Statement parameters                                        {#parameters}
====================

In an SQL statement (including a prepared statemen), you may write special
_placeholders_ in the query text.  They look like `$1`, `$2`, and so on.

When executing the query later, you pass parameter values.  The call will
respectively substitute the first parameter value where it finds `$1` in the
query, the second where it finds `$2`, _et cetera._

For example, let's say you have a transaction called `tx`.  Here's how you
execute a plain statement:

```cxx
pqxx::result r = tx.exec("SELECT name FROM employee where id=101");
```

Inserting the `101` in there is awkward and even dangerous.  We'll get to that
in a moment.  Here's how you do it better, using parameters:

```cxx
pqxx::result r = tx.exec("SELECT name FROM employee WHERE id=$1", {101});
```

That second argument to `exec()`, the `{101}`, constructs a `pqxx::params`
object.  The `exec()` call will fill this value in where the query says `$1`.

Doing this saves you work.  If you don't use statement parameters, you'll need
to quote and escape your values (see `connection::quote()` and friends) as you
insert them into your query as literal values.

Or if you forget to do that, you leave yourself open to horrible
[SQL injection attacks](https://xkcd.com/327/).  Trust me, I was born in a town
whose name started with an apostrophe!

With parameters you can pass your values as they are, and they will go across
the wire to the database in a safe format.

In some cases it may even be faster!  When a parameter represents binary data
(as in the SQL `BYTEA` type), libpqxx will send it directly as binary, which is
a bit more efficient than the standard textual format in which the data
normally gets sent to the database.  If you insert the binary data directly in
your query text, your CPU will have some extra work to do, converting the data
into a text format, escaping it, and adding quotes; and the data will take up
more bytes, which take time to transmit.


Multiple parameters
-------------------

The `pqxx::params` class is quite fleixble.  It can contain any number of
parameter values, of many different types.

You can pass them in while constructing the `params` object:

```cxx
pqxx::params{23, "acceptance", 3.14159}
```

Or you can add them one by one:

```cxx
pqxx::params p;
p.append(23);
p.append("acceptance");
p.append(3.14159);
```

You can also combine the two, passing some values int the constructor and
adding the rest later.  You can even insert a `params` into a `params`:

```cxx
pqxx::params p{23};
p.append(params{"acceptance", 3.14159});
```

Each of these examples will produce the same list of parameters.


Generating placeholders
-----------------------

If your code gets particularly complex, it may sometimes happen that it becomes
hard to track which parameter value belongs with which placeholder.  Did you
intend to pass this numeric value as `$7`, or as `$8`?  The answer may depend
on an `if` that happened earlier in a different function.

(Generally if things get that complex, it's a good idea to look for simpler
solutions.  But especially when performance matters, sometimes you can't avoid
complexity like that.)

There's a little helper class called `placeholders`.  You can use it as a
counter which produces those placeholder strings, `$1`, `$2`, `$3`, et cetera.
When you start generating a complex statement, you can create both a `params`
and a `placeholders`:

```cxx
    pqxx::params values;
    pqxx::placeholders name;
```

Let's say you've got some complex code to generate the conditions for an SQL
"WHERE" clause.  You'll generally want to do these things close together in
your, so that you don't accidentally update one part and forget another:

```cxx
    if (extra_clause)
    {
      // Extend the query text, using the current placeholder.
      query += " AND x = " + name.get();
      // Add the parameter value.
      values.append(my_x);
      // Move on to the next placeholder value.
      name.next();
    }
```

Depending on the starting value of `name`, this might add to `query` a fragment
like " `AND x = $3` " or " `AND x = $5` ".
