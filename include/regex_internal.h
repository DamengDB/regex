/* Extended regular expression matching and search library.

   Copyright 2017-2025 Dameng Database Co.,Ltd.

   Copyright (C) 2002-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Isamu Hasegawa <isamu@yamato.ibm.com>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef _REGEX_INTERNAL_H
#define _REGEX_INTERNAL_H 1

#include "regex.h"
#include <limits.h>

#include <assert.h>
#include <ctype.h>
//#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <stdint.h>

/* bug585036
* 放开两个头文件的引用，确保is_utf8相关代码中函数nl_langinfo的执行
*/
#ifdef __GNUC__
#include <langinfo.h>
#include <stdbool.h>
//#include <intprops.h>
#endif

#ifdef _LIBC
# include <libc-lock.h>
# define lock_define(name) __libc_lock_define (, name)
# define lock_init(lock) (__libc_lock_init (lock), 0)
# define lock_fini(lock) ((void) 0)
# define lock_lock(lock) __libc_lock_lock (lock)
# define lock_unlock(lock) __libc_lock_unlock (lock)
#elif defined GNULIB_LOCK && !defined USE_UNLOCKED_IO
# include "glthread/lock.h"
  /* Use gl_lock_define if empty macro arguments are known to work.
     Otherwise, fall back on less-portable substitutes.  */
# if ((defined __GNUC__ && !defined __STRICT_ANSI__) \
      || (defined __STDC_VERSION__ && 199901L <= __STDC_VERSION__))
#  define lock_define(name) gl_lock_define (, name)
# elif USE_POSIX_THREADS
#  define lock_define(name) pthread_mutex_t name;
# elif USE_PTH_THREADS
#  define lock_define(name) pth_mutex_t name;
# elif USE_SOLARIS_THREADS
#  define lock_define(name) mutex_t name;
# elif USE_WINDOWS_THREADS
#  define lock_define(name) gl_lock_t name;
# else
#  define lock_define(name)
# endif
# define lock_init(lock) glthread_lock_init (&(lock))
# define lock_fini(lock) glthread_lock_destroy (&(lock))
# define lock_lock(lock) glthread_lock_lock (&(lock))
# define lock_unlock(lock) glthread_lock_unlock (&(lock))
#elif defined GNULIB_PTHREAD && !defined USE_UNLOCKED_IO
# include <pthread.h>
# define lock_define(name) pthread_mutex_t name;
# define lock_init(lock) pthread_mutex_init (&(lock), 0)
# define lock_fini(lock) pthread_mutex_destroy (&(lock))
# define lock_lock(lock) pthread_mutex_lock (&(lock))
# define lock_unlock(lock) pthread_mutex_unlock (&(lock))
#else
# define lock_define(name)
# define lock_init(lock) 0
# define lock_fini(lock) ((void) 0)
  /* The 'dfa' avoids an "unused variable 'dfa'" warning from GCC.  */
# define lock_lock(lock) ((void) dfa)
# define lock_unlock(lock) ((void) 0)
#endif

/* In case that the system doesn't have isblank().  */
#if !defined _LIBC && ! (defined isblank || (HAVE_ISBLANK && HAVE_DECL_ISBLANK))
# define isblank(ch) ((ch) == ' ' || (ch) == '\t')
#endif

#ifdef _LIBC
# ifndef _RE_DEFINE_LOCALE_FUNCTIONS
#  define _RE_DEFINE_LOCALE_FUNCTIONS 1
#   include <locale/localeinfo.h>
#   include <locale/coll-lookup.h>
# endif
#endif

/* This is for other GNU distributions with internationalized messages.  */
#if (HAVE_LIBINTL_H && ENABLE_NLS) || defined _LIBC
# include <libintl.h>
# ifdef _LIBC
#  undef gettext
#  define gettext(msgid) \
  __dcgettext (_libc_intl_domainname, msgid, LC_MESSAGES)
# endif
#else
# undef gettext
# define gettext(msgid) (msgid)
#endif

#ifndef gettext_noop
/* This define is so xgettext can find the internationalizable
   strings.  */
# define gettext_noop(String) String
#endif

#if (defined MB_CUR_MAX && HAVE_WCTYPE_H && HAVE_ISWCTYPE) || _LIBC
# define RE_ENABLE_I18N
#endif

/* Number of ASCII characters.  */
#define ASCII_CHARS 0x80

/* Number of single byte characters.  */
#define SBC_MAX (UCHAR_MAX + 1)

#define COLL_ELEM_LEN_MAX 8

/* The character which represents newline.  */
#define NEWLINE_CHAR '\n'
#define WIDE_NEWLINE_CHAR L'\n'

/* Rename to standard API for using out of glibc.  */
#ifndef _LIBC
# undef __wctype
# undef __iswalnum
# undef __iswctype
# undef __towlower
# undef __towupper
# define __wctype wctype
# define __iswalnum iswalnum
# define __iswctype iswctype
# define __towlower towlower
# define __towupper towupper
# define __btowc btowc
# define __mbrtowc mbrtowc
# define __wcrtomb wcrtomb
# define __regfree regfree
# define __mempcpy mempcpy
# define __wcscoll wcscoll
# define __iswlower iswlower
#endif /* not _LIBC */

#ifndef _LIBC
#ifndef __attribute_warn_unused_result__
# define __attribute_warn_unused_result__
#endif
#ifndef inline
# define inline
#endif
#endif /* not _LIBC */

/* For loser systems without the definition.  */
#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif

#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t) (SIZE_MAX / 2))
#endif

/* The type of indexes into strings.  This is signed, not size_t,
   since the API requires indexes to fit in regoff_t anyway, and using
   signed integers makes the code a bit smaller and presumably faster.
   The traditional GNU regex implementation uses int for indexes.
   The POSIX-compatible implementation uses a possibly-wider type.
   The name 'Idx' is three letters to minimize the hassle of
   reindenting a lot of regex code that formerly used 'int'.  */
typedef regoff_t Idx;
#ifdef _REGEX_LARGE_OFFSETS
# define IDX_MAX SSIZE_MAX
#else
# define IDX_MAX INT_MAX
#endif

/* A hash value, suitable for computing hash tables.  */
typedef __re_size_t re_hashval_t;

//#ifndef __GNUC__
# define __attribute__(arg)

/* True if the real type T is signed.  */
// #define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))

// regex 2.26
// #if __GNUC__ >= 3
// # define BE(expr, val) __builtin_expect (expr, val)
// #else
// # define BE(expr, val) (expr)
//#endif

// regex 2.30
// # define __glibc_unlikely(expr) __builtin_expect (expr, 0)
#define __glibc_unlikely(expr) (expr)

// # define __glibc_likely(expr) __builtin_expect (expr, 1)
#define __glibc_likely(expr) (expr)

//#endif  // end __GNUC__

#ifndef bool
#define bool    int
#define false   (0)
#define true    (~0)
#endif

/* An integer used to represent a set of bits.  It must be unsigned,
   and must be at least as wide as unsigned int.  */
typedef unsigned long int bitset_word_t;

#ifndef CHAR_BIT
/* Number of bits in a 'char'.	*/
#define CHAR_BIT    8
#endif
/* All bits set in a bitset_word_t.  */
#define BITSET_WORD_MAX ULONG_MAX

/* Number of bits in a bitset_word_t.  For portability to hosts with
   padding bits, do not use '(sizeof (bitset_word_t) * CHAR_BIT)';
   instead, deduce it directly from BITSET_WORD_MAX.  Avoid
   greater-than-32-bit integers and unconditional shifts by more than
   31 bits, as they're not portable.  */
#if BITSET_WORD_MAX == 0xffffffffUL
# define BITSET_WORD_BITS 32
#elif BITSET_WORD_MAX >> 31 >> 4 == 1
# define BITSET_WORD_BITS 36
#elif BITSET_WORD_MAX >> 31 >> 16 == 1
# define BITSET_WORD_BITS 48
#elif BITSET_WORD_MAX >> 31 >> 28 == 1
# define BITSET_WORD_BITS 60
#elif BITSET_WORD_MAX >> 31 >> 31 >> 1 == 1
# define BITSET_WORD_BITS 64
#elif BITSET_WORD_MAX >> 31 >> 31 >> 9 == 1
# define BITSET_WORD_BITS 72
#elif BITSET_WORD_MAX >> 31 >> 31 >> 31 >> 31 >> 3 == 1
# define BITSET_WORD_BITS 128
#elif BITSET_WORD_MAX >> 31 >> 31 >> 31 >> 31 >> 31 >> 31 >> 31 >> 31 >> 7 == 1
# define BITSET_WORD_BITS 256
#elif BITSET_WORD_MAX >> 31 >> 31 >> 31 >> 31 >> 31 >> 31 >> 31 >> 31 >> 7 > 1
# define BITSET_WORD_BITS 257 /* any value > SBC_MAX will do here */
# if BITSET_WORD_BITS <= SBC_MAX
#  error "Invalid SBC_MAX"
# endif
#else
# error "Add case for new bitset_word_t size"
#endif

/* Number of bitset_word_t values in a bitset_t.  */
#define BITSET_WORDS ((SBC_MAX + BITSET_WORD_BITS - 1) / BITSET_WORD_BITS)

typedef bitset_word_t bitset_t[BITSET_WORDS];
typedef bitset_word_t *re_bitset_ptr_t;
typedef const bitset_word_t *re_const_bitset_ptr_t;

#define PREV_WORD_CONSTRAINT 0x0001
#define PREV_NOTWORD_CONSTRAINT 0x0002
#define NEXT_WORD_CONSTRAINT 0x0004
#define NEXT_NOTWORD_CONSTRAINT 0x0008
#define PREV_NEWLINE_CONSTRAINT 0x0010
#define NEXT_NEWLINE_CONSTRAINT 0x0020
#define PREV_BEGBUF_CONSTRAINT 0x0040
#define NEXT_ENDBUF_CONSTRAINT 0x0080
#define WORD_DELIM_CONSTRAINT 0x0100
#define NOT_WORD_DELIM_CONSTRAINT 0x0200

typedef enum
{
  INSIDE_WORD = PREV_WORD_CONSTRAINT | NEXT_WORD_CONSTRAINT,
  WORD_FIRST = PREV_NOTWORD_CONSTRAINT | NEXT_WORD_CONSTRAINT,
  WORD_LAST = PREV_WORD_CONSTRAINT | NEXT_NOTWORD_CONSTRAINT,
  INSIDE_NOTWORD = PREV_NOTWORD_CONSTRAINT | NEXT_NOTWORD_CONSTRAINT,
  LINE_FIRST = PREV_NEWLINE_CONSTRAINT,
  LINE_LAST = NEXT_NEWLINE_CONSTRAINT,
  BUF_FIRST = PREV_BEGBUF_CONSTRAINT,
  BUF_LAST = NEXT_ENDBUF_CONSTRAINT,
  WORD_DELIM = WORD_DELIM_CONSTRAINT,
  NOT_WORD_DELIM = NOT_WORD_DELIM_CONSTRAINT
} re_context_type;

typedef struct
{
  Idx alloc;
  Idx nelem;
  Idx *elems;
} re_node_set;

typedef enum
{
  NON_TYPE = 0,

  /* Node type, These are used by token, node, tree.  */
  CHARACTER = 1,
  END_OF_RE = 2,
  SIMPLE_BRACKET = 3,
  OP_BACK_REF = 4,
  OP_PERIOD = 5,
#ifdef RE_ENABLE_I18N
  COMPLEX_BRACKET = 6,
  OP_UTF8_PERIOD = 7,
#endif /* RE_ENABLE_I18N */

  /* We define EPSILON_BIT as a macro so that OP_OPEN_SUBEXP is used
     when the debugger shows values of this enum type.  */
#define EPSILON_BIT 8
  OP_OPEN_SUBEXP = EPSILON_BIT | 0,
  OP_CLOSE_SUBEXP = EPSILON_BIT | 1,
  OP_ALT = EPSILON_BIT | 2,
  OP_DUP_ASTERISK = EPSILON_BIT | 3,
  ANCHOR = EPSILON_BIT | 4,

  /* Tree type, these are used only by tree. */
  CONCAT = 16,
  SUBEXP = 17,

  /* Token type, these are used only by token.  */
  OP_DUP_PLUS = 18,
  OP_DUP_QUESTION,
  OP_OPEN_BRACKET,
  OP_CLOSE_BRACKET,
  OP_CHARSET_RANGE,
  OP_OPEN_DUP_NUM,
  OP_CLOSE_DUP_NUM,
  OP_NON_MATCH_LIST,
  OP_OPEN_COLL_ELEM,
  OP_CLOSE_COLL_ELEM,
  OP_OPEN_EQUIV_CLASS,
  OP_CLOSE_EQUIV_CLASS,
  OP_OPEN_CHAR_CLASS,
  OP_CLOSE_CHAR_CLASS,
  OP_WORD,
  OP_NOTWORD,
  OP_SPACE,
  OP_NOTSPACE,
  BACK_SLASH

} re_token_type_t;

#ifdef RE_ENABLE_I18N
extern const bitset_t utf8_sb_map;

typedef struct
{
  /* Multibyte characters.  */
  wchar_t *mbchars;

  /* Collating symbols.  */
# ifdef _LIBC
  int32_t *coll_syms;
# endif

  /* Equivalence classes. */
# ifdef _LIBC
  int32_t *equiv_classes;
# endif

  /* Range expressions. */
# ifdef _LIBC
  uint32_t *range_starts;
  uint32_t *range_ends;
# else /* not _LIBC */
  wchar_t *range_starts;
  wchar_t *range_ends;
# endif /* not _LIBC */

  /* Character classes. */
  wctype_t *char_classes;

  /* If this character set is the non-matching list.  */
  unsigned int non_match : 1;

  /* # of multibyte characters.  */
  Idx nmbchars;

  /* # of collating symbols.  */
  Idx ncoll_syms;

  /* # of equivalence classes. */
  Idx nequiv_classes;

  /* # of range expressions. */
  Idx nranges;

  /* # of character classes. */
  Idx nchar_classes;
} re_charset_t;
#endif /* RE_ENABLE_I18N */

typedef struct
{
  union
  {
    unsigned char c;		/* for CHARACTER */
    re_bitset_ptr_t sbcset;	/* for SIMPLE_BRACKET */
#ifdef RE_ENABLE_I18N
    re_charset_t *mbcset;	/* for COMPLEX_BRACKET */
#endif /* RE_ENABLE_I18N */
    Idx idx;			/* for BACK_REF */
    re_context_type ctx_type;	/* for ANCHOR */
  } opr;
#if __GNUC__ >= 2 && !defined __STRICT_ANSI__
  re_token_type_t type : 8;
#else
  re_token_type_t type;
#endif
  unsigned int constraint : 10;	/* context constraint */
  unsigned int duplicated : 1;
  unsigned int opt_subexp : 1;
#ifdef RE_ENABLE_I18N
  unsigned int accept_mb : 1;
  /* These 2 bits can be moved into the union if needed (e.g. if running out
     of bits; move opr.c to opr.c.c and move the flags to opr.c.flags).  */
  unsigned int mb_partial : 1;
#endif
  unsigned int word_char : 1;
} re_token_t;

#define IS_EPSILON_NODE(type) ((type) & EPSILON_BIT)

struct re_string_t
{
  /* Indicate the raw buffer which is the original string passed as an
     argument of regexec(), re_search(), etc..  */
  const unsigned char *raw_mbs;
  /* Store the multibyte string.  In case of "case insensitive mode" like
     REG_ICASE, upper cases of the string are stored, otherwise MBS points
     the same address that RAW_MBS points.  */
  unsigned char *mbs;
#ifdef RE_ENABLE_I18N
  /* Store the wide character string which is corresponding to MBS.  */
  wint_t *wcs;
  Idx *offsets;
  mbstate_t cur_state;
#endif
  /* Index in RAW_MBS.  Each character mbs[i] corresponds to
     raw_mbs[raw_mbs_idx + i].  */
  Idx raw_mbs_idx;
  /* The length of the valid characters in the buffers.  */
  Idx valid_len;
  /* The corresponding number of bytes in raw_mbs array.  */
  Idx valid_raw_len;
  /* The length of the buffers MBS and WCS.  */
  Idx bufs_len;
  /* The index in MBS, which is updated by re_string_fetch_byte.  */
  Idx cur_idx;
  /* length of RAW_MBS array.  */
  Idx raw_len;
  /* This is RAW_LEN - RAW_MBS_IDX + VALID_LEN - VALID_RAW_LEN.  */
  Idx len;
  /* End of the buffer may be shorter than its length in the cases such
     as re_match_2, re_search_2.  Then, we use STOP for end of the buffer
     instead of LEN.  */
  Idx raw_stop;
  /* This is RAW_STOP - RAW_MBS_IDX adjusted through OFFSETS.  */
  Idx stop;

  /* The context of mbs[0].  We store the context independently, since
     the context of mbs[0] may be different from raw_mbs[0], which is
     the beginning of the input string.  */
  unsigned int tip_context;
  /* The translation passed as a part of an argument of re_compile_pattern.  */
  RE_TRANSLATE_TYPE trans;
  /* Copy of re_dfa_t's word_char.  */
  re_const_bitset_ptr_t word_char;
  /* true if REG_ICASE.  */
  unsigned char icase;
  unsigned char is_utf8;
  unsigned char map_notascii;
  unsigned char mbs_allocated;
  unsigned char offsets_needed;
  unsigned char newline_anchor;
  unsigned char word_ops_used;
  int mb_cur_max;
};
typedef struct re_string_t re_string_t;


struct re_dfa_t;
typedef struct re_dfa_t re_dfa_t;

struct reg_mem_t
{
    mem_malloc_t    mem_malloc;
    mem_realloc_t   mem_realloc;
    mem_free_t      mem_free;
    mem_calloc_t    mem_calloc;
};
typedef struct reg_mem_t reg_mem_t;

//674671 用函数取&g_reg_mem影响性能改为extern g_reg_mem
extern reg_mem_t g_reg_mem;

#ifndef _LIBC
# define IS_IN(libc) false
#endif

#ifndef _LIBC
# ifdef __i386__
#  define internal_function   __attribute__ ((regparm (3), stdcall))
# else
#  define internal_function
# endif
#endif

#ifndef NOT_IN_libc
reg_errcode_t re_string_realloc_buffers (re_string_t *pstr,
						Idx new_buf_len);
#ifdef RE_ENABLE_I18N
void build_wcs_buffer (re_string_t *pstr);
reg_errcode_t build_wcs_upper_buffer (re_string_t *pstr);
#endif /* RE_ENABLE_I18N */
void build_upper_buffer (re_string_t *pstr);
void re_string_translate_buffer (re_string_t *pstr);
unsigned int re_string_context_at (const re_string_t *input, Idx idx,
					  int eflags) __attribute__ ((pure));
#endif

#define re_string_peek_byte(pstr, offset) \
  ((pstr)->mbs[(pstr)->cur_idx + offset])
#define re_string_fetch_byte(pstr) \
  ((pstr)->mbs[(pstr)->cur_idx++])
#define re_string_first_byte(pstr, idx) \
  ((idx) == (pstr)->valid_len || (pstr)->wcs[idx] != WEOF)
#define re_string_is_single_byte_char(pstr, idx) \
  ((pstr)->wcs[idx] != WEOF && ((pstr)->valid_len == (idx) + 1 \
				|| (pstr)->wcs[(idx) + 1] != WEOF))
#define re_string_eoi(pstr) ((pstr)->stop <= (pstr)->cur_idx)
#define re_string_cur_idx(pstr) ((pstr)->cur_idx)
#define re_string_get_buffer(pstr) ((pstr)->mbs)
#define re_string_length(pstr) ((pstr)->len)
#define re_string_byte_at(pstr,idx) ((pstr)->mbs[idx])
#define re_string_skip_bytes(pstr,idx) ((pstr)->cur_idx += (idx))
#define re_string_set_index(pstr,idx) ((pstr)->cur_idx = (idx))

#if defined _LIBC || HAVE_ALLOCA
# include <alloca.h>
#endif

#ifndef _LIBC
# if HAVE_ALLOCA
/* The OS usually guarantees only one guard page at the bottom of the stack,
   and a page size can be as small as 4096 bytes.  So we cannot safely
   allocate anything larger than 4096 bytes.  Also care for the possibility
   of a few compiler-allocated temporary stack slots.  */
#  define __libc_use_alloca(n) ((n) < 4032)
# else
/* alloca is implemented with malloc, so just use malloc.  */
#  define __libc_use_alloca(n) 0
#  undef alloca
#  define alloca(n) malloc (n)
# endif
#endif

#ifdef _LIBC
# define MALLOC_0_IS_NONNULL 1
#elif !defined MALLOC_0_IS_NONNULL
# define MALLOC_0_IS_NONNULL 0
#endif

#ifndef MAX
# define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif
#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

// SMR-陶志远-20241227-004 有外部提供的内存函数使用外部的，否则使用系统函数
#define re_malloc(t,n) (g_reg_mem.mem_malloc == NULL ? (t *) malloc ((n) * sizeof (t)) : (t *) g_reg_mem.mem_malloc((n) * sizeof (t)))
#define re_realloc(p,t,n) (g_reg_mem.mem_realloc == NULL ? (t *) realloc (p, (n) * sizeof (t)) : (t *) g_reg_mem.mem_realloc(p, (n) * sizeof (t)))
#define re_free(p) {if (NULL != p) g_reg_mem.mem_free == NULL ? free (p) : g_reg_mem.mem_free(p);}
#define re_calloc(t, n) (g_reg_mem.mem_calloc == NULL ? calloc (t, n) : g_reg_mem.mem_calloc (t, n))

/* 下列代码仅供dm内存泄漏分析使用 */
// #define re_malloc(t,n) ((t *) dm_malloc ((n) * sizeof (t)))
// #define re_realloc(p,t,n) ((t *) dm_realloc (p, (n) * sizeof (t)))
// #define re_free(p) {if (NULL != p) dm_free (p);}
// #define re_calloc(t, n) (dm_calloc (t, n))
// 
// static inline void*
// dm_malloc(size_t  n)
// {
//     void*       dst;
// 
//     dst = malloc (n);
// 
// #ifdef _DEBUG
//     fprintf(stderr, "malloc: %p, %d\n", dst, n);
// #endif
// 
//     return dst;
// }
// 
// static inline void*
// dm_realloc(void*   p, size_t  n)
// {
//     void*       dst;
// 
//     dst = realloc (p, n);
// 
// #ifdef _DEBUG
//     fprintf(stderr, "realloc: old: %p, new: %p, %d\n", p, dst, n);
// #endif
// 
//     return dst;
// }
// 
// static inline void
// dm_free(void*   p)
// {
// #ifdef _DEBUG
//     fprintf(stderr, "free : %p\n", p);
// #endif
// 
//     free (p);
// }
// 
// /* calloc会将申请的空间初始化0 */
// static inline void*
// dm_calloc(size_t  t, size_t n)
// {
//     void*       dst;
// 
//     dst = calloc (t, n);
// 
// #ifdef _DEBUG
//     fprintf(stderr, "calloc: %p, %d\n", dst, t * n);
// #endif
// 
//     return dst;
// }

struct bin_tree_t
{
  struct bin_tree_t *parent;
  struct bin_tree_t *left;
  struct bin_tree_t *right;
  struct bin_tree_t *first;
  struct bin_tree_t *next;

  re_token_t token;

  /* 'node_idx' is the index in dfa->nodes, if 'type' == 0.
     Otherwise 'type' indicate the type of this node.  */
  Idx node_idx;
};
typedef struct bin_tree_t bin_tree_t;

#define BIN_TREE_STORAGE_SIZE \
  ((1024 - sizeof (void *)) / sizeof (bin_tree_t))

struct bin_tree_storage_t
{
  struct bin_tree_storage_t *next;
  bin_tree_t data[BIN_TREE_STORAGE_SIZE];
};
typedef struct bin_tree_storage_t bin_tree_storage_t;

#define CONTEXT_WORD 1
#define CONTEXT_NEWLINE (CONTEXT_WORD << 1)
#define CONTEXT_BEGBUF (CONTEXT_NEWLINE << 1)
#define CONTEXT_ENDBUF (CONTEXT_BEGBUF << 1)

#define IS_WORD_CONTEXT(c) ((c) & CONTEXT_WORD)
#define IS_NEWLINE_CONTEXT(c) ((c) & CONTEXT_NEWLINE)
#define IS_BEGBUF_CONTEXT(c) ((c) & CONTEXT_BEGBUF)
#define IS_ENDBUF_CONTEXT(c) ((c) & CONTEXT_ENDBUF)
#define IS_ORDINARY_CONTEXT(c) ((c) == 0)

#define IS_WORD_CHAR(ch) (isalnum (ch) || (ch) == '_')
#define IS_NEWLINE(ch) ((ch) == NEWLINE_CHAR)
#define IS_WIDE_WORD_CHAR(ch) (__iswalnum (ch) || (ch) == L'_')
#define IS_WIDE_NEWLINE(ch) ((ch) == WIDE_NEWLINE_CHAR)

#define NOT_SATISFY_PREV_CONSTRAINT(constraint,context) \
 ((((constraint) & PREV_WORD_CONSTRAINT) && !IS_WORD_CONTEXT (context)) \
  || ((constraint & PREV_NOTWORD_CONSTRAINT) && IS_WORD_CONTEXT (context)) \
  || ((constraint & PREV_NEWLINE_CONSTRAINT) && !IS_NEWLINE_CONTEXT (context))\
  || ((constraint & PREV_BEGBUF_CONSTRAINT) && !IS_BEGBUF_CONTEXT (context)))

#define NOT_SATISFY_NEXT_CONSTRAINT(constraint,context) \
 ((((constraint) & NEXT_WORD_CONSTRAINT) && !IS_WORD_CONTEXT (context)) \
  || (((constraint) & NEXT_NOTWORD_CONSTRAINT) && IS_WORD_CONTEXT (context)) \
  || (((constraint) & NEXT_NEWLINE_CONSTRAINT) && !IS_NEWLINE_CONTEXT (context)) \
  || (((constraint) & NEXT_ENDBUF_CONSTRAINT) && !IS_ENDBUF_CONTEXT (context)))

struct re_dfastate_t
{
  re_hashval_t hash;
  re_node_set nodes;
  re_node_set non_eps_nodes;
  re_node_set inveclosure;
  re_node_set *entrance_nodes;
  struct re_dfastate_t **trtable, **word_trtable;
  unsigned int context : 4;
  unsigned int halt : 1;
  /* If this state can accept "multi byte".
     Note that we refer to multibyte characters, and multi character
     collating elements as "multi byte".  */
  unsigned int accept_mb : 1;
  /* If this state has backreference node(s).  */
  unsigned int has_backref : 1;
  unsigned int has_constraint : 1;
};
typedef struct re_dfastate_t re_dfastate_t;

struct re_state_table_entry
{
  Idx num;
  Idx alloc;
  re_dfastate_t **array;
};

/* Array type used in re_sub_match_last_t and re_sub_match_top_t.  */

typedef struct
{
  Idx next_idx;
  Idx alloc;
  re_dfastate_t **array;
} state_array_t;

/* Store information about the node NODE whose type is OP_CLOSE_SUBEXP.  */

typedef struct
{
  Idx node;
  Idx str_idx; /* The position NODE match at.  */
  state_array_t path;
} re_sub_match_last_t;

/* Store information about the node NODE whose type is OP_OPEN_SUBEXP.
   And information about the node, whose type is OP_CLOSE_SUBEXP,
   corresponding to NODE is stored in LASTS.  */

typedef struct
{
  Idx str_idx;
  Idx node;
  state_array_t *path;
  Idx alasts; /* Allocation size of LASTS.  */
  Idx nlasts; /* The number of LASTS.  */
  re_sub_match_last_t **lasts;
} re_sub_match_top_t;

struct re_backref_cache_entry
{
  Idx node;
  Idx str_idx;
  Idx subexp_from;
  Idx subexp_to;
  char more;
  char unused;
  unsigned short int eps_reachable_subexps_map;
};

typedef struct
{
  /* The string object corresponding to the input string.  */
  re_string_t input;
#if defined _LIBC || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L)
  const re_dfa_t *const dfa;
#else
  const re_dfa_t *dfa;
#endif
  /* EFLAGS of the argument of regexec.  */
  int eflags;
  /* Where the matching ends.  */
  Idx match_last;
  Idx last_node;
  /* The state log used by the matcher.  */
  re_dfastate_t **state_log;
  Idx state_log_top;
  /* Back reference cache.  */
  Idx nbkref_ents;
  Idx abkref_ents;
  struct re_backref_cache_entry *bkref_ents;
  int max_mb_elem_len;
  Idx nsub_tops;
  Idx asub_tops;
  re_sub_match_top_t **sub_tops;
} re_match_context_t;

typedef struct
{
  re_dfastate_t **sifted_states;
  re_dfastate_t **limited_states;
  Idx last_node;
  Idx last_str_idx;
  re_node_set limits;
} re_sift_context_t;

struct re_fail_stack_ent_t
{
  Idx idx;
  Idx node;
  regmatch_t *regs;
  re_node_set eps_via_nodes;
};

struct re_fail_stack_t
{
  Idx num;
  Idx alloc;
  struct re_fail_stack_ent_t *stack;
};

struct re_dfa_t
{
  re_token_t *nodes;
  size_t nodes_alloc;
  size_t nodes_len;
  Idx *nexts;
  Idx *org_indices;
  re_node_set *edests;
  re_node_set *eclosures;
  re_node_set *inveclosures;
  struct re_state_table_entry *state_table;
  re_dfastate_t *init_state;
  re_dfastate_t *init_state_word;
  re_dfastate_t *init_state_nl;
  re_dfastate_t *init_state_begbuf;
  bin_tree_t *str_tree;
  bin_tree_storage_t *str_tree_storage;
  re_bitset_ptr_t sb_char;
  int str_tree_storage_idx;

  /* number of subexpressions 're_nsub' is in regex_t.  */
  re_hashval_t state_hash_mask;
  Idx init_node;
  Idx nbackref; /* The number of backreference in this dfa.  */

  /* Bitmap expressing which backreference is used.  */
  bitset_word_t used_bkref_map;
  bitset_word_t completed_bkref_map;

  unsigned int has_plural_match : 1;
  /* If this dfa has "multibyte node", which is a backreference or
     a node which can accept multibyte character or multi character
     collating element.  */
  unsigned int has_mb_node : 1;
  unsigned int is_utf8 : 1;
  unsigned int map_notascii : 1;
  unsigned int word_ops_used : 1;
  int mb_cur_max;
  bitset_t word_char;
  reg_syntax_t syntax;
  Idx *subexp_map;
#ifdef DEBUG
  char* re_str;
#endif
  lock_define (lock)
};

#define re_node_set_init_empty(set) memset (set, '\0', sizeof (re_node_set))
#define re_node_set_remove(set,id) \
  (re_node_set_remove_at (set, re_node_set_contains (set, id) - 1))
#define re_node_set_empty(p) ((p)->nelem = 0)
#define re_node_set_free(set) re_free ((set)->elems)


typedef enum
{
  SB_CHAR,
  MB_CHAR,
  EQUIV_CLASS,
  COLL_SYM,
  CHAR_CLASS
} bracket_elem_type;

typedef struct
{
  bracket_elem_type type;
  union
  {
    unsigned char ch;
    unsigned char *name;
    wchar_t wch;
  } opr;
} bracket_elem_t;


/* Functions for bitset_t operation.  */

static inline void
bitset_set (bitset_t set, Idx i)
{
  set[i / BITSET_WORD_BITS] |= (bitset_word_t) 1 << i % BITSET_WORD_BITS;
}

static inline void
bitset_clear (bitset_t set, Idx i)
{
  set[i / BITSET_WORD_BITS] &= ~ ((bitset_word_t) 1 << i % BITSET_WORD_BITS);
}

static inline bool
bitset_contain (const bitset_t set, Idx i)
{
  return (set[i / BITSET_WORD_BITS] >> i % BITSET_WORD_BITS) & 1;
}

static inline void
bitset_empty (bitset_t set)
{
  memset (set, '\0', sizeof (bitset_t));
}

static inline void
bitset_set_all (bitset_t set)
{
  memset (set, -1, sizeof (bitset_word_t) * (SBC_MAX / BITSET_WORD_BITS));
  if (SBC_MAX % BITSET_WORD_BITS != 0)
    set[BITSET_WORDS - 1] =
      ((bitset_word_t) 1 << SBC_MAX % BITSET_WORD_BITS) - 1;
}

static inline void
bitset_copy (bitset_t dest, const bitset_t src)
{
  memcpy (dest, src, sizeof (bitset_t));
}

static inline void
bitset_not (bitset_t set)
{
  int bitset_i;
  for (bitset_i = 0; bitset_i < SBC_MAX / BITSET_WORD_BITS; ++bitset_i)
    set[bitset_i] = ~set[bitset_i];
  if (SBC_MAX % BITSET_WORD_BITS != 0)
    set[BITSET_WORDS - 1] =
      ((((bitset_word_t) 1 << SBC_MAX % BITSET_WORD_BITS) - 1)
       & ~set[BITSET_WORDS - 1]);
}

static inline void
bitset_merge (bitset_t dest, const bitset_t src)
{
  int bitset_i;
  for (bitset_i = 0; bitset_i < BITSET_WORDS; ++bitset_i)
    dest[bitset_i] |= src[bitset_i];
}

static inline void
bitset_mask (bitset_t dest, const bitset_t src)
{
  int bitset_i;
  for (bitset_i = 0; bitset_i < BITSET_WORDS; ++bitset_i)
    dest[bitset_i] &= src[bitset_i];
}

re_dfastate_t *
__attribute_warn_unused_result__
re_acquire_state_context (reg_errcode_t *err, const re_dfa_t *dfa,
			  const re_node_set *nodes, unsigned int context);

bool
__attribute_warn_unused_result__
re_node_set_insert_last (re_node_set *set, Idx elem);

reg_errcode_t
__attribute_warn_unused_result__
re_node_set_alloc (re_node_set *set, Idx size);

unsigned char
__attribute__ ((pure))
re_string_peek_byte_case (const re_string_t *pstr, Idx idx);

unsigned char
re_string_fetch_byte_case (re_string_t *pstr);

unsigned int
re_string_context_at (const re_string_t *input, Idx idx, int eflags);

void
re_string_destruct (re_string_t *pstr);

bool
__attribute_warn_unused_result__
re_node_set_insert (re_node_set *set, Idx elem);

Idx
__attribute__ ((pure))
re_node_set_contains (const re_node_set *set, Idx elem);

reg_errcode_t
__attribute_warn_unused_result__
re_node_set_init_copy (re_node_set *dest, const re_node_set *src);

reg_errcode_t
__attribute_warn_unused_result__
re_node_set_init_1 (re_node_set *set, Idx elem);

reg_errcode_t
__attribute_warn_unused_result__
re_node_set_init_union (re_node_set *dest, const re_node_set *src1,
			const re_node_set *src2);

re_dfastate_t *
__attribute_warn_unused_result__
re_acquire_state (reg_errcode_t *err, const re_dfa_t *dfa,
		  const re_node_set *nodes);

reg_errcode_t
__attribute_warn_unused_result__
re_node_set_merge (re_node_set *dest, const re_node_set *src);

reg_errcode_t
__attribute_warn_unused_result__
re_node_set_add_intersect (re_node_set *dest, const re_node_set *src1,
			   const re_node_set *src2);

void
re_node_set_remove_at (re_node_set *set, Idx idx);

void
free_state (re_dfastate_t *state);

reg_errcode_t
__attribute_warn_unused_result__
re_string_construct (re_string_t *pstr, const char *str, Idx len,
		     RE_TRANSLATE_TYPE trans, bool icase, const re_dfa_t *dfa);

Idx
re_dfa_add_node (re_dfa_t *dfa, re_token_t token);

reg_errcode_t
__attribute_warn_unused_result__
re_node_set_init_2 (re_node_set *set, Idx elem1, Idx elem2);

reg_errcode_t
__attribute_warn_unused_result__
re_string_allocate (re_string_t *pstr, const char *str, Idx len, Idx init_len,
		    RE_TRANSLATE_TYPE trans, bool icase, const re_dfa_t *dfa);

reg_errcode_t
__attribute_warn_unused_result__
re_string_reconstruct (re_string_t *pstr, Idx idx, int eflags);

#ifdef RE_ENABLE_I18N
/* Functions for re_string.  */
static int
__attribute__ ((pure, unused))
re_string_char_size_at (const re_string_t *pstr, Idx idx)
{
  int byte_idx;
  if (pstr->mb_cur_max == 1)
    return 1;
  for (byte_idx = 1; idx + byte_idx < pstr->valid_len; ++byte_idx)
    if (pstr->wcs[idx + byte_idx] != WEOF)
      break;
  return byte_idx;
}

static wint_t
__attribute__ ((pure, unused))
re_string_wchar_at (const re_string_t *pstr, Idx idx)
{
  if (pstr->mb_cur_max == 1)
    return (wint_t) pstr->mbs[idx];
  return (wint_t) pstr->wcs[idx];
}

# ifdef _LIBC
#  include <locale/weight.h>
# endif

static int
__attribute__ ((pure, unused))
re_string_elem_size_at (const re_string_t *pstr, Idx idx)
{
# ifdef _LIBC
  const unsigned char *p, *extra;
  const int32_t *table, *indirect;
  uint_fast32_t nrules = _NL_CURRENT_WORD (LC_COLLATE, _NL_COLLATE_NRULES);

  if (nrules != 0)
    {
      table = (const int32_t *) _NL_CURRENT (LC_COLLATE, _NL_COLLATE_TABLEMB);
      extra = (const unsigned char *)
	_NL_CURRENT (LC_COLLATE, _NL_COLLATE_EXTRAMB);
      indirect = (const int32_t *) _NL_CURRENT (LC_COLLATE,
						_NL_COLLATE_INDIRECTMB);
      p = pstr->mbs + idx;
      findidx (table, indirect, extra, &p, pstr->len - idx);
      return p - pstr->mbs - idx;
    }
  else
# endif /* _LIBC */
    return 1;
}
#endif /* RE_ENABLE_I18N */

#ifndef FALLTHROUGH
# if __GNUC__ < 7
#  define FALLTHROUGH ((void) 0)
# else
#  define FALLTHROUGH __attribute__ ((__fallthrough__))
# endif
#endif

#endif /*  _REGEX_INTERNAL_H */
