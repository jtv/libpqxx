String escaping                      {#escaping}
===============

Writing queries as strings is easy.  But sometimes you need a variable in
there: `"SELECT id FROM user WHERE name = '" + name + "'"`.

This is dangerous.  See the bug?  If `name` can contain quotes, you may have
an SQL injection vulnerability there, where users can enter nasty stuff like
"`.'; DROP TABLE user`".  Or if you're lucky, it's just a nasty bug that you
discover when `name` happens to be "d'Arcy".  Or...  Well, I was born in a
place called _'s-Gravenhage..._

There are two ways of dealing with this.  One is statement _parameters:_ some
SQL execution functions in libpqxx let you write placeholders for such values
in your SQL, like `$1`, `$2`, etc.  When you then pass your variables as the
parameter values, they get substituted into the query, but in a safe form.

The other is to _escape_ the values yourself, before inserting them into your
SQL.  This isn't as safe as using parameters, since you need to be really
conscientious about it.  Use parameters if you can... and libpqxx will do the
escaping for you.

In escaping, quotes and other problematic characters are marked as "this is
just a character inside the string, not the end of the string."  There are
[several functions](@ref escaping-functions) in libpqxx to do this for you.


SQL injection
-------------

To understand what SQL injection vulnerabilities are and why they should be
prevented, imagine you use the following SQL statement somewhere in your
program:

```cxx
    tx.exec(
        "SELECT number, amount "
        "FROM account "
        "WHERE allowed_to_see('" + userid + "','" + password + "')");
```

This shows a logged-in user important information on all accounts he is
authorized to view.  The userid and password strings are variables entered
by the user himself.

Now, if the user is actually an attacker who knows (or can guess) the
general shape of this SQL statement, imagine getting following password:

```text
    x') OR ('x' = 'x
```

Does that make sense to you?  Probably not.  But if this is inserted into
the SQL string by the C++ code above, the query becomes:

```sql
    SELECT number, amount
    FROM account
    WHERE allowed_to_see('user','x') OR ('x' = 'x')
```

Is this what you wanted to happen?  Probably not!  The neat `allowed_to_see()`
clause is completely circumvented by the "`OR ('x' = 'x')`" clause, which is
always `true`.  Therefore, the attacker will get to see all accounts in the
database!


Using the esc functions
-----------------------

Here's how you can fix the problem in the example above:

```cxx
    tx.exec(
        "SELECT number, amount "
        "FROM account "
        "WHERE allowed_to_see('" + tx.esc(userid) + "', "
        "'" + tx.esc(password) + "')");
```

Now, the quotes embedded in the attacker's string will be neatly escaped so
they can't "break out" of the quoted SQL string they were meant to go into:

```sql
    SELECT number, amount
    FROM account
    WHERE allowed_to_see('user', 'x'') OR (''x'' = ''x')
```

If you look carefully, you'll see that thanks to the added escape characters
(a single-quote is escaped in SQL by doubling it) all we get is a very
strange-looking password string--but not a change in the SQL statement.

In practice, of course, it would be better to use parameters:

```cxx
    tx.exec_params(
        " SELECT number, amount "
        "FROM account "
        "WHERE allowed_to_see($1, $2)",
	userid, password);
```
