// Test for non-standard "__restrict" keyword.
/** This keyword seems to exist in gcc and Visual Studio, with an effect
 * similar to that of C's "restrict" keyword.
 *
 * We can't do much with this keyword, sadly, because there's no way to apply
 * it to a view or a span, only to a raw pointer.  We no longer use a lot of
 * raw pointers in libpqxx.
 */
char const *__restrict x;
