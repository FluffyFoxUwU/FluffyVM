#include <FluffyGC/v1.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

#include "bug.h"
#include "vm_array_primitive.h"
#include "vm.h"
#include "value.h"
#include "util.h"
#include "constants.h"

static const char* subystemKey = "I'm a";

#define get_private(F) (F->array_primitiveData)

int array_primitive_init(struct vm* F) {
  F->array_primitiveData = malloc(sizeof(*F->array_primitiveData));
  if (!F->array_primitiveData)
    return -ENOMEM;
  
  fluffygc_field fields[] = {
    {
      .name = "actualArray",
      .offset = offsetof(struct primitive_array, actualArray),
      .dataType = FLUFFYGC_TYPE_NORMAL,
      .type = FLUFFYGC_FIELD_STRONG
    }
  };
  fluffygc_descriptor_args tmp = {
    .name = VM_PACKAGE ".PrimitiveArray",
    .objectSize = sizeof(struct primitive_array),
    .fieldCount = ARRAY_SIZE(fields),
    .fields = fields,
    .typeID = (uintptr_t) &subystemKey 
  };
  
  get_private(F)->desc = fluffygc_v1_descriptor_new(F->heap, &tmp);
  if (!get_private(F)->desc)
    return -ENOMEM;
  
  return 0;
}

void array_primitive_cleanup(struct vm* F) {
  if (!get_private(F))
    return;
  
  fluffygc_v1_descriptor_delete(F->heap, get_private(F)->desc);
  free(get_private(F));
}

struct array_primitive_gcobject* array_primitive_new(struct vm* F, size_t len) {
  fluffygc_object* self = fluffygc_v1_new_object(F->heap, get_private(F)->desc);
  if (!self)
    return NULL;
  
  fluffygc_object* actualArrayObj = fluffygc_v1_new_opaque_object(F->heap, sizeof(struct value) * len);
  if (!actualArrayObj)
    goto failure;
  fluffygc_v1_set_object_field(F->heap, self, offsetof(struct primitive_array, actualArray), actualArrayObj);
  fluffygc_v1_obj_write_data(F->heap, self, offsetof(struct primitive_array, len), FIELD_SIZEOF(struct primitive_array, len), &len);
  fluffygc_v1_delete_local_ref(F->heap, actualArrayObj);
  
  return (struct array_primitive_gcobject*) self;
  
failure:
  fluffygc_v1_delete_local_ref(F->heap, self);
  return NULL;
}

size_t array_primitive_get_length(struct vm* F, struct array_primitive_gcobject* self) {
  size_t len;
  fluffygc_v1_obj_read_data(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, len), sizeof(len), &len);
  return len;
}

int array_primitive_set(struct vm* F, struct array_primitive_gcobject* self, int index, struct value data) {
  printf("W: %d\n", index);
  size_t len;
  fluffygc_v1_obj_read_data(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, len), sizeof(len), &len);
  if (index >= len)
    return -ERANGE;
  
  if (index < 0 || value_is_byref(F, data))
    return -EINVAL;
  
  fluffygc_object* array = fluffygc_v1_get_object_field(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, actualArray));
  if (!array)
    return -ENOMEM;
  
  fluffygc_v1_obj_write_data(F->heap, array, sizeof(struct value) * index, sizeof(data), &data);
  fluffygc_v1_delete_local_ref(F->heap, array);
  return 0;
}

int array_primitive_get(struct vm* F, struct array_primitive_gcobject* self, int index, struct value* result) {
  printf("R: %d\n", index);
  
  size_t len;
  fluffygc_v1_obj_read_data(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, len), sizeof(len), &len);
  if (index >= len)
    return -ERANGE;
  
  if (!result)
    return 0;
  
  fluffygc_object* array = fluffygc_v1_get_object_field(F->heap, cast_to_gcobj(self), offsetof(struct primitive_array, actualArray));  return 0;
  if (!array)
    return -ENOMEM;
  
  fluffygc_v1_obj_read_data(F->heap, array, sizeof(struct value) * index, sizeof(*result), result);
  fluffygc_v1_delete_local_ref(F->heap, array);
  return result->type;
}
