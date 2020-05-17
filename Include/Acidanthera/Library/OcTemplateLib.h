/** @file

OcTemplateLib

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_TEMPLATE_LIB_H
#define OC_TEMPLATE_LIB_H

#include <Library/BaseMemoryLib.h>

//
// Common structor prototype.
//
typedef
VOID
(*OC_STRUCTOR) (
  VOID    *Ptr,
  UINT32  Size
  );

//
// Private primitives for type generations
//

#define PRIV_OC_STRUCTOR_IGNORE(...)
#define PRIV_OC_STRUCTOR_EXPAND(...) __VA_ARGS__

#define PRIV_OC_SELECT_NEXT_INNER(Dummy, Next, ...) Next
//
// Without this layer of indirection, MSVC evaluates __VA_ARGS__ early and
// PRIV_OC_SELECT_NEXT_INNER receives only two arguments, the second always
// being Unused as per PRIV_OC_SELECT_NEXT.
//
#define PRIV_OC_SELECT_NEXT_INNER_INDIR(...) PRIV_OC_SELECT_NEXT_INNER __VA_ARGS__
#define PRIV_OC_SELECT_NEXT(...) PRIV_OC_SELECT_NEXT_INNER_INDIR((__VA_ARGS__, Unused))
#define PRIV_OC_REMOVE_NEXT(...) , do { } while (0),

#define PRIV_OC_DECLARE_STRUCT_MEMBER(Type, Name, Suffix, Constructor, Destructor)   \
  Type Name Suffix;

#define PRIV_OC_CONSTRUCT_STRUCT_MEMBER(Type, Name, Suffix, Constructor, Destructor) \
  .Name = Constructor,

#define PRIV_OC_DESTRUCT_STRUCT_MEMBER(Type, Name, Suffix, Constructor, Destructor)  \
  PRIV_OC_SELECT_NEXT(PRIV_OC_REMOVE_NEXT Destructor, Destructor(&Obj->Name, sizeof (Type Suffix)));

#define PRIV_OC_INVOKE_DESTRUCTOR(Destructor, Obj, Size)                             \
  PRIV_OC_SELECT_NEXT(PRIV_OC_REMOVE_NEXT Destructor, Destructor(Obj, Size))

#define PRIV_NO_KEY_TYPE UINT8

//
// Declare structure type NAME. Construction and destruction interfaces will also be
// available with corresponding prototypes:
// VOID NAME_CONSTRUCT(Void *Ptr, UINTN Size);
// VOID NAME_DESTRUCT(Void *Ptr, UINTN Size);
//
// Field information will be retrieved from NAME_FIELDS X-Macro, written as follows:
#if 0
#define NAME_FIELDS(_, __) \
  _(Type, Name, Type Suffix, __(Constant Initializer), Destructor Function) \
  _(Type, Name, Type Suffix, OC_CONSTR(Type, _, __), OC_DESTR(Type))
#endif
//

#define OC_DECLARE(Name)                                                           \
  typedef struct Name ## _ {                                                       \
    Name ## _FIELDS (PRIV_OC_DECLARE_STRUCT_MEMBER, PRIV_OC_STRUCTOR_IGNORE)       \
  } Name;                                                                          \
  VOID Name ## _CONSTRUCT(VOID *Ptr, UINT32 Size);                                 \
  VOID Name ## _DESTRUCT(VOID *Ptr, UINT32 Size);

//
// Generate constructors and destructors for structure type NAME.
// Second argument permits to do extra actions prior to field destruction.
// Pass () to the second argument to ignore this option.
//
#define OC_STRUCTORS(Name, Destructor)                                             \
  VOID Name ## _CONSTRUCT(VOID *Ptr, UINT32 Size) {                                \
    STATIC Name Obj = {                                                            \
      Name ## _FIELDS(PRIV_OC_CONSTRUCT_STRUCT_MEMBER, PRIV_OC_STRUCTOR_EXPAND)    \
    };                                                                             \
    CopyMem (Ptr, &Obj, sizeof (Name));                                            \
  }                                                                                \
  VOID Name ## _DESTRUCT(VOID *Ptr, UINT32 Size) {                                 \
    Name  *Obj = (Name *) Ptr;  (VOID) Obj;                                        \
    PRIV_OC_INVOKE_DESTRUCTOR(Destructor, Obj, sizeof (Name));                     \
    Name ## _FIELDS(PRIV_OC_DESTRUCT_STRUCT_MEMBER, PRIV_OC_STRUCTOR_EXPAND)       \
  }

//
// Use previously defined structure constructor or destructor
// FIXME: We need to use these recursively, and normally one would implement
// it with deferred execution, like this:
// #define OC_CONSTR_REAL(A, _, __) ({A ## _FIELDS(_, __)})
// #define OC_CONSTR_REAL_ID() OC_CONSTR_REAL
// #define OC_CONSTR DEFER(OC_CONSTR_REAL_ID)()
// However, this will not work in this exact case, as _, which itself is a macro,
// changes the expansion order. The right fix here is to entirely remove the mess
// and use external tool for template generation.
//
#define OC_CONSTR(A, _, __)  {A ## _FIELDS(_, __)}
#define OC_CONSTR1(A, _, __) {A ## _FIELDS(_, __)}
#define OC_CONSTR2(A, _, __) {A ## _FIELDS(_, __)}
#define OC_CONSTR3(A, _, __) {A ## _FIELDS(_, __)}
#define OC_CONSTR5(A, _, __) {A ## _FIELDS(_, __)}
#define OC_DESTR(A) A ## _DESTRUCT

//
// Generate array-like blob (string, data, metadata) of type Type,
// Count static size, MaxSize is real size, and default value Default.
//
#define OC_BLOB(Type, Count, Default, _, __) \
  _(UINT32       , Size      ,       , 0                   , OcZeroField   ) \
  _(UINT32       , MaxSize   ,       , sizeof (Type Count) , OcZeroField   ) \
  _(Type *       , DynValue  ,       , NULL                , OcFreePointer ) \
  _(Type         , Value     , Count , __(Default)         , ()            )

#define OC_BLOB_STRUCTORS(Name) \
  OC_STRUCTORS(Name, ())

#define OC_BLOB_CONSTR(Type, Constructor, SizeConstructor, _, __) \
  __({.Size = SizeConstructor, .MaxSize = sizeof (((Type *)0)->Value), .DynValue = NULL, .Value = Constructor})

//
// Generate map-like container with key elements of type KeyType, OC_BLOB derivative,
// value types of Type constructed by Constructor and destructed by Destructor:
#if 0
#define CONT_FIELDS(_, __) \
  OC_MAP (KEY, ELEM, _, __)
  OC_DECLARE (CONT)
#endif
//

#define OC_MAP(KeyType, Type, _, __) \
  _(UINT32      , Count         , , 0                     , () ) \
  _(UINT32      , AllocCount    , , 0                     , () ) \
  _(OC_STRUCTOR , Construct     , , Type ## _CONSTRUCT    , () ) \
  _(OC_STRUCTOR , Destruct      , , Type ## _DESTRUCT     , () ) \
  _(Type **     , Values        , , NULL                  , () ) \
  _(UINT32      , ValueSize     , , sizeof (Type)         , () ) \
  _(UINT32      , KeySize       , , sizeof (KeyType)      , () ) \
  _(OC_STRUCTOR , KeyConstruct  , , KeyType ## _CONSTRUCT , () ) \
  _(OC_STRUCTOR , KeyDestruct   , , KeyType ## _DESTRUCT  , () ) \
  _(KeyType **  , Keys          , , NULL                  , () )

#define OC_MAP_STRUCTORS(Name) \
  OC_STRUCTORS(Name, OcFreeMap)

//
// Generate array-like container with elements of type Type, constructed
// by Constructor and destructed by Destructor.
#if 0
#define CONT_FIELDS(_, __) \
  OC_ARRAY (ELEM, _, __)
  OC_DECLARE (CONT)
#endif
//

#define OC_ARRAY(Type, _, __) \
  _(UINT32      , Count         , , 0                     , () ) \
  _(UINT32      , AllocCount    , , 0                     , () ) \
  _(OC_STRUCTOR , Construct     , , Type ## _CONSTRUCT    , () ) \
  _(OC_STRUCTOR , Destruct      , , Type ## _DESTRUCT     , () ) \
  _(Type **     , Values        , , NULL                  , () ) \
  _(UINT32      , ValueSize     , , sizeof (Type)         , () )

#define OC_ARRAY_STRUCTORS(Name) \
  OC_STRUCTORS(Name, OcFreeArray)

//
// Free dynamically allocated memory if non NULL.
// Note, that the first argument is actually VOID **.
//
VOID
OcFreePointer (
  VOID    *Pointer,
  UINT32  Size
  );

//
// Zero field memory.
//
VOID
OcZeroField (
  VOID    *Pointer,
  UINT32  Size
  );

//
// Do not invoke any actions at destruction.
//
VOID
OcDestructEmpty (
  VOID    *Pointer,
  UINT32  Size
  );

//
// OC_MAP-like destructor.
//
VOID
OcFreeMap (
  VOID    *Pointer,
  UINT32  Size
  );

//
// OC_ARRAY-like destructor.
//
VOID
OcFreeArray (
  VOID    *Pointer,
  UINT32  Size
  );

//
// Prepare memory for blob at Pointer to contain Size bytes.
// Assignable size may be returned via OutSize.
// This method guarantees not to overwrite "Default" value,
// but may destroy the previous value.
// NULL is returned on allocation failure.
//
VOID *
OcBlobAllocate (
  VOID    *Pointer,
  UINT32  Size,
  UINT32  **OutSize  OPTIONAL
  );

//
// Obtain blob value
//
#define OC_BLOB_GET(Blob) (((Blob)->DynValue) != NULL ? ((Blob)->DynValue) : ((Blob)->Value))

//
// Insert new empty element into the OC_MAP or OC_ARRAY, depending
// on Key value.
//
BOOLEAN
OcListEntryAllocate (
  VOID            *Pointer,
  VOID            **Value,
  VOID            **Key
  );

//
// Some useful generic types
// OC_STRING  - implements support for resizable ASCII strings.
// OC_DATA    - implements support for resizable data blobs.
// OC_ASSOC   - implements support for maps with ASCII keys and data values.
//
#define OC_STRING_FIELDS(_, __) \
  OC_BLOB (CHAR8, [64], {0}, _, __)
  OC_DECLARE (OC_STRING)

#define OC_STRING_CONSTR(Constructor, _, __) \
  OC_BLOB_CONSTR (OC_STRING, __(Constructor), sizeof (Constructor), _, __)

#define OC_ESTRING_CONSTR(_, __) \
  OC_BLOB_CONSTR (OC_STRING, __(""), 0, _, __)

#define OC_DATA_FIELDS(_, __) \
  OC_BLOB (UINT8, [64], {0}, _, __)
  OC_DECLARE (OC_DATA)

#define OC_EDATA_CONSTR(_, __) \
  OC_BLOB_CONSTR (OC_DATA, __({0}), 0, _, __)

#define OC_DATA_CONSTR(Constructor, _, __) \
  OC_BLOB_CONSTR (OC_DATA, __(Constructor), sizeof ((UINT8[]) Constructor), _, __)

#define OC_ASSOC_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_DATA, _, __)
  OC_DECLARE (OC_ASSOC)

#endif // OC_TEMPLATE_LIB_H
