# Mool

a simple memory pool to enhance performance.

---

## The main idea
is to limit as much as possible the number of memory allocations through
**malloc()**, **calloc()** or **realloc()**, since these functions use system call internally, they can be quite expensive, 
and memory on the heap is quite slow, the Mool library allocates a big chunk of memory
once and the API provides the functions to request and return memory to the big allocation.

---

## Fall back system
the API ensure to always allocate memory, if there is not enough space in the memory chuck,
or the user is requesting a bigger memory chunk, the system fall back 
to the use of **malloc()**, the programmer doesn't need to know if the memory is allocated 
from the original chunck,or  a new malloced piece, the system will keep truck of the memory and free accordingly, 
when the user will call **cancel_memory()** function from the API. 

---

## Security 
since the **malloc()** behavior is sometimes weird and puzzling,
depending on the platform that you are on, this sofwtare desing try to enahnce memory security 
by putting a limit to the memory using a page size buffer after the allocated memory,
ensuring that, if the program overflows the **MEM_SIZE**, will always crash.
if you use **close_prog_memory()** before terminating your programs, memory leaks are avoided, sinxe this
function will free all the memory that has been allocated via the API.

---
