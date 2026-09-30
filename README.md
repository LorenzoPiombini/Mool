# Mool -- The Memory allocator of Doom

this is the 'zone allocator' used in [Doom](https://github.com/id-Software/DOOM).

## Few differences
I did not modify a whole lot from the source code, I added a small macro for the memory allignment, and pritty much it.
In the source code, if you pass a pointer that does not belong to the memory block, the zone allocator will throw an error and 
crash the program, this code will just return without a crash.

I did not use all the tag names that they decided to use in the game.

there is no, `close_zone()` function because the zone allocator, and there is no need for it.
this arena is design to be the only memory that your program will need during the execution, so there is no need to free it, OS will do
it for you, once the program exit.
