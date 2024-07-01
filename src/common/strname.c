#include "pocketpy/common/strname.h"
#include "pocketpy/common/smallmap.h"
#include "pocketpy/common/utils.h"
#include "pocketpy/common/vector.h"
#include "pocketpy/pocketpy.h"

#include <stdio.h>

// TODO: use a more efficient data structure
static c11_smallmap_s2n _interned;
static c11_vector /*T=char* */ _r_interned;
static bool _initialized = false;

void pk_StrName__initialize() {
    if(_initialized) return;
    c11_smallmap_s2n__ctor(&_interned);
    for(int i = 0; i < _r_interned.count; i++) {
        free(c11__at(char*, &_r_interned, i));
    }
    c11_vector__ctor(&_r_interned, sizeof(c11_string));
    _initialized = true;

#define MAGIC_METHOD(x) x = pk_StrName__map(#x);
#include "pocketpy/xmacros/magics.h"
#undef MAGIC_METHOD

    // print all names
    for(int i = 0; i < _interned.count; i++) {
        printf("%d: %s\n", i + 1, c11__getitem(char*, &_r_interned, i));
    }

    pk_id_add = pk_StrName__map("add");
    pk_id_set = pk_StrName__map("set");
    pk_id_long = pk_StrName__map("long");
    pk_id_complex = pk_StrName__map("complex");
}

void pk_StrName__finalize() {
    if(!_initialized) return;
    // free all char*
    for(int i = 0; i < _r_interned.count; i++) {
        free(c11__getitem(char*, &_r_interned, i));
    }
    c11_smallmap_s2n__dtor(&_interned);
    c11_vector__dtor(&_r_interned);
}

uint16_t pk_StrName__map(const char* name) {
    return pk_StrName__map2((c11_string){name, strlen(name)});
}

uint16_t pk_StrName__map2(c11_string name) {
    // TODO: PK_GLOBAL_SCOPE_LOCK()
    if(!_initialized) {
        pk_StrName__initialize();  // lazy init
    }
    uint16_t index = c11_smallmap_s2n__get(&_interned, name, 0);
    if(index != 0) return index;
    // generate new index
    if(_interned.count > 65530) { PK_FATAL_ERROR("StrName index overflow\n"); }
    // NOTE: we must allocate the string in the heap so iterators are not invalidated
    char* p = malloc(name.size + 1);
    memcpy(p, name.data, name.size);
    p[name.size] = '\0';
    c11_vector__push(char*, &_r_interned, p);
    index = _r_interned.count;  // 1-based
    // save to _interned
    c11_smallmap_s2n__set(&_interned, (c11_string){p, name.size}, index);
    assert(_interned.count == _r_interned.count);
    return index;
}

const char* pk_StrName__rmap(uint16_t index) {
    assert(_initialized);
    assert(index > 0 && index <= _interned.count);
    return c11__getitem(char*, &_r_interned, index - 1);
}

c11_string pk_StrName__rmap2(uint16_t index) {
    const char* p = pk_StrName__rmap(index);
    return (c11_string){p, strlen(p)};
}

py_Name py_name(const char* name) {
    return pk_StrName__map(name);
}

const char* py_name2str(py_Name name) {
    return pk_StrName__rmap(name);
}

bool py_ismagicname(py_Name name){
    return name <= __missing__;
}

///////////////////////////////////
#define MAGIC_METHOD(x) uint16_t x;
#include "pocketpy/xmacros/magics.h"
#undef MAGIC_METHOD

uint16_t pk_id_add;
uint16_t pk_id_set;
uint16_t pk_id_long;
uint16_t pk_id_complex;
