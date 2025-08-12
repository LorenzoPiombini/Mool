## Mool

a simple memory pool to enhance performance.

---

# The main idea
is to limit as much as possible the number of memory allocations through
malloc calloc or realloc, since these functions use system call internally, they can be quite expensive, 
and memory on the heap is quite slow, the Mool library allocates a big chunk of memory
once and the API provides the functions to request and return memory to the big allocation.
