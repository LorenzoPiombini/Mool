# Mool -- The Memory allocator of Doom

this is the 'zone allocator' used in [Doom](https://github.com/id-Software/DOOM).

## Few differences
I did not modify a whole lot from the source code, I added a small macro for the memory allignment, and that is it, in the source code.
In the original code, if you pass a pointer that does not belong to the memory block, the zone allocator will throw an error and 
crash the program. **Mool** will just return without a crash.
 ** *Realloc* ** -> I added A_Realloc, which is simply a wrap function on `A_Malloc` and `A_free`, it works like realloc in `libc`, with the only difference, that
 A_Realloc is only make the memory size bigger it does not shrink the memory size. 

I did not use all the tag names that they decided to use in the game.

there is no, `close_zone()` function because the zone allocator, and there is no need for it.
this arena is design to be the only memory that your program will need during the execution, so there is no need to free it, OS will do
it for you, once the program exit.

## Build
the code works on all the three major platform, the make file, as is, is for linux, but with a few changes, compiles and work just fine on MacOS and Windows)
