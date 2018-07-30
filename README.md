
[![GitHub license](https://img.shields.io/github/license/icedac/potio.svg?style=flat-square)](./LICENSE)


# [WORK-IN-PROGRESS] np_alloc
- yet-another, per-thread memory alloc/pool for c++
- code from scratch, some idea from written commerical code by myself in 2009.
  
## toolset
- c++14/gsl/x64
- consideration for linux, not yet buildable
 

## feature
- lock-free, per-size, per-thread memory pool
 
## api
```
void*    np_alloc(size_t bytes);
void*    np_alloc(size_t bytes, const char file[], int line);
void     np_free(void * ptr);
```

## usage & test code

see [unittest.cpp](https://github.com/icedac/np_alloc/blob/master/test/unittest/UnitTest.cpp)
 
## pros / cons
- (+) no lock to get new memory so fast
- (+) improved cache locality due to pre-allocated by memory page for local thread
- (-) using lots of memory but can be optimizing per project
 

## todo
### garbage collecting process from local thread pool to global pool
- it will saves memory but worse cache localtity due to spreading out memory fragments across threads

## benchmark

comming...
