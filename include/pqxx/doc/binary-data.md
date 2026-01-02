Binary data                                                        {#binary}
===========

The database has two ways of storing binary data: `BYTEA` is like a string, but
containing bytes rather than text characters.  And _large objects_ are more
like a separate table containing binary objects.

Generally you'll want to use `BYTEA` for reasonably-sized values, and large
objects for very large values.

That's the database side.  On the C++ side, in libpqxx, all binary data must be
some block of contiguous `std::byte` values in memory.  That could be a
`std::vector<std::byte>`, or `std::span<std::byte>`, and so on.  However the
_preferred_ types for binary data in libpqxx are...

* `pqxx::bytes` for storing the data, similar to `std::string` for text.
* `pqxx::bytes_view` for reading data stored elsewhere, similar to how you'd
   use `std::string_view` for text.
* `pqxx::writable_bytes_view` for writing to data stored elsewhere.

So for example, if you want to write a large object, you'd create a
`pqxx::blob` object.  You might use that to write data which you pass in the
form of a `pqxx::bytes_view`.  You might then read that data back by letting
`pqxx::blob` write the data into a `pqxx::bytes &` or a
`pqxx::writable_bytes_view` that you give it.

So long as it's _basically_ still a block of bytes though, you can use
`pqxx::binary_cast` to construct a `pqxx::bytes_view` from it:

```cxx
    std::string hi{"Hello binary world"};
    my_blob.write(pqxx::binary_cast(hi);
```

For convenience there's also a form of `binary_cast` that takes a pointer and
a length.

```cxx
    char const greeting[] = "Hello binary world";
    char const *hi = greeting;
    my_blob.write(pqxx::binary_cast(hi, sizeof(greeting)));
```
