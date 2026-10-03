# Mool -- The Memory allocator of Doom

this is the 'zone allocator' used in [Doom](https://github.com/id-Software/DOOM).

## Few differences
I did not modify a whole lot from the source code, I added a macro for the memory allignment.
In the original code, if you pass a pointer that does not belong to the memory block, the zone allocator will throw an error and 
crash the program. **Mool** will just return without a crash.  
### ** *Realloc* **   
I added `A_Realloc`, which is simply a wrap function on `A_Malloc` and `A_free`, it works like realloc in `libc`, with the only difference that,
`A_Realloc` only makes the memory size bigger it does not shrink the memory size.
The ownership detection of a block of memory is different than what used originally.


I did not use all the tag names that they decided to use in the game.


## Build
the code works on all the three major platform, the make file, as is, is for linux, but with a few changes, compiles and work just fine on MacOS and Windows)
