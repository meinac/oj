#include "cache.h"
#include "oj.h"
#include "parser.h"
#include "usual.h"

#define BYTE_OFFSETS_STACK_INC_SIZE 256

// Holds the start byte offsets of each JSON object encountered
typedef struct _byte_offsets {
    int   length;
    int   current;
    long *stack;
} ByteOffsets;

typedef struct _introspect_S {
  struct _usual usual; // inherit all the attributes from `_usual` struct

  ByteOffsets *byte_offsets;

  void (*delegated_open_object_func)(struct _ojParser *p);
  void (*delegated_open_object_key_func)(struct _ojParser *p);
  void (*delegated_close_object_func)(struct _ojParser *p);
} *IntrospectDelegate;

static void dfree(ojParser p) {
    Usual d = (Usual)p->ctx;

    cache_free(d->str_cache);
    cache_free(d->attr_cache);
    if (NULL != d->sym_cache) {
        cache_free(d->sym_cache);
    }
    if (NULL != d->class_cache) {
        cache_free(d->class_cache);
    }
    xfree(d->vhead);
    xfree(d->chead);
    xfree(d->khead);
    xfree(d->create_id);
    xfree(p->ctx);
    p->ctx = NULL;
}

static void open_object_introspected(ojParser p) {
  IntrospectDelegate d = (IntrospectDelegate)p->ctx;

  d->delegated_open_object_func(p);
}

static void open_object_key_introspected(ojParser p) {
  IntrospectDelegate d = (IntrospectDelegate)p->ctx;

  d->delegated_open_object_key_func(p);
}

static void close_object_introspected(ojParser p) {
  IntrospectDelegate d = (IntrospectDelegate)p->ctx;

  d->delegated_close_object_func(p);
}

static void init_introspect_parser(ojParser p) {
  IntrospectDelegate d = ALLOC(struct _introspect_S);

  oj_init_usual(p, (Usual)d);

  p->ctx  = (void *)d; // point to the introspect delegate(Do we need this? It will be always the same address as usual is at the start of the IntrospectDelegate.)
  p->free = dfree; // replace this

  // // now function mangling...
  Funcs f = &p->funcs[TOP_FUN];
  d->delegated_open_object_func = f->open_object;
  d->delegated_close_object_func = f->close_object;
  f->open_object  = open_object_introspected;
  f->close_object = close_object_introspected;

  f = &p->funcs[ARRAY_FUN];
  f->open_object  = open_object_key_introspected;
  f->close_object = close_object_introspected;

  f = &p->funcs[OBJECT_FUN];
  d->delegated_open_object_key_func = f->open_object;
  f->open_object  = open_object_key_introspected;
  f->close_object = close_object_introspected;
}

VALUE oj_get_parser_introspect() {
  VALUE oj_parser = oj_parser_new();
  struct _ojParser *p;
  Data_Get_Struct(oj_parser, struct _ojParser, p);

  init_introspect_parser(p);

  rb_gc_register_address(&oj_parser);

  return oj_parser;
}
