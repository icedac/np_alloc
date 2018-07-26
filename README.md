
[![GitHub license](https://img.shields.io/github/license/icedac/potio.svg?style=flat-square)](./LICENSE)


# [WORK-IN-PROGRESS] np_alloc
- generic per-thread memory alloc/pool for c++

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
