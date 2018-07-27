
[![GitHub license](https://img.shields.io/github/license/icedac/potio.svg?style=flat-square)](./LICENSE)


# [WORK-IN-PROGRESS] np_alloc
- yet-another, per-thread memory alloc/pool for c++
- code from scratch, some idea from written commerical code by myself in 2009.
  
## toolset
- c++14/gsl


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

## benchmark

comming...
