/**
*
* Title: CDL Object File Format.
*
* Date:  June 1988
*
*        (c) Copyright 1988, Perihelion Software Ltd.
*
*        All Rights Reserved.
*
* $Header: /hsrc/cmds/cdl/RCS/cdlobj.h,v 1.2 1993/04/14 16:38:33 nickc Exp $
*
**/

#define TYPE_1_OBJ 0x12345678
#define TYPE_2_OBJ 0x12345677
#define TYPE_3_OBJ 0x12345676

#define TF_INCLUDED 0x00000001
#define SF_EXTERNAL 0x80000000
#define SF_CREATE   O_Create

typedef struct S_INDEX
{
  char *ptr;
  WORD index;
} S_INDEX;

typedef struct A_INDEX
{
  WORD index;
  LIST list;
} A_INDEX;

typedef struct I_INDEX
{
  WORD index;
  struct CDL_ISTREAM *ptr;
} I_INDEX;

typedef struct CDL_HEADER
{
  WORD type;
  WORD nocomponents;
  WORD nocstreams;
  WORD noistreams;
  WORD noattribs;
  S_INDEX currentdir;
  S_INDEX tf_name;
} CDL_HEADER;

typedef struct CDL_DEV_ATTR
{
  NODE node;
  WORD count;
  S_INDEX attribute;
} CDL_DEV_ATTR;

typedef struct CDL_COMPONENT
{
  S_INDEX name;
  WORD flags;
  Object *toobj;
  S_INDEX puid;
  PTYPE p_type;
  WORD noattribs;
  A_INDEX p_attrib;
  UWORD memory;
  LIFE longevity;
  UWORD time;
  WORD priority;
  WORD nargs;
  S_INDEX args;
  WORD noistreams;
  I_INDEX istreams;
} CDL_COMPONENT;

typedef struct CDL_ISTREAM
{
  WORD index;
  WORD mode;
  WORD standard;
} CDL_ISTREAM;

typedef struct CDL_CSTREAM
{
  S_INDEX name;
  WORD flags;
  WORD count;
} CDL_CSTREAM;

typedef struct CDL_STREAMS
{
  CDL_CSTREAM cstreams[1];
  CDL_ISTREAM istreams[4];
} CDL_STREAMS;

typedef struct CDL_STRINGS
{
  WORD length;
  char text[1];
} STRINGS;
