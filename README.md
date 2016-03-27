# Introduction

sfpool (Simple Fast Pool) is a another memory pool allocator
library written in C99 (if its a library at all) .

* iterator object (you can walk through allocated blocks of memory pool)

# What is a memory pool?

Memory pools, also called fixed-size blocks allocation, is the use of
pools for memory management that allows dynamic memory allocation comparable
to malloc or C++'s operator new. As those implementations suffer from
fragmentation because of variable block sizes, it is not recommendable
to use them in a real time system due to performance. A more efficient
solution is preallocating a number of memory blocks with the same size
called the memory pool.
(from https://en.wikipedia.org/wiki/Memory_pool)
