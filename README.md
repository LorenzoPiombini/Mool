# Mool

a simple memory pool to enhance performance.

---

## The main idea
is to limit as much as possible the number of memory allocations through
**malloc()**, **calloc()** or **realloc()**, since these functions use system call internally, they can be quite expensive, 
and memory on the heap is quite slow, the Mool library allocates a big chunk of memory
once and the API provides the functions to request and return memory to the big allocation.

## Fall back system
the API ensure to always allocates memory, if there is not enough space in the memory chuck,
or the user is requesting a bigger memory chunck, then the system fall back 
to the use of **malloc()**, and the programmer won't know if the memory is allocated 
from the original chunck, the system will keep truck of the memory and free accordingly, 
when the user will call **cancel_memory()** function from the 
Mool API. 
