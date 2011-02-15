/* By Brice Arnould <unbrice@vleu.net>
 * Copyright (C) 2011 Gandi SAS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* Enables KSM on heap-allocated memory.
 * Usage: make
 *        LD_PRELOAD+=./ksm-preload.so command args ...
 */


#define _GNU_SOURCE		// dlsym(), mremap()
#include <dlfcn.h>		// dlsym()
#include <sys/mman.h>		// mmap(), mmap2(), mremap()
#include <unistd.h>		// syscall()
#include <sys/syscall.h>	// SYS_mmap, SYS_mmap2

#include <assert.h>
#include <error.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>		// fprintf(), stderr
#include <stdint.h>		// uintptr_t
#include <stdlib.h>

/* The default value for merge_threshold */
#define DEFAULT_MERGE_THRESHOLD (4086*8)
static const char *const MERGE_THRESHOLD_ENV_NAME = "KSMP_MERGE_THRESHOLD";

/* mmap2 is only defined on some 32 bit systems
 * (actually, it might only be defined i386)
 */
#ifdef SYS_mmap2
# define MMAP2_ENABLED 1
#else
# define MMAP2_ENABLED 0
#endif



/******** GLOBAL STATE ********/

/* Aliases for function types.
 * Just a little spoon of syntactic sugar to help the medicine go down
 */
typedef void *calloc_function (size_t nmemb, size_t size);
typedef void *malloc_function (size_t size);
typedef void *mmap_function (void *start, size_t length, int prot, int flags,
			     int fd, off_t offset);
typedef void *mmap2_function (void *start, size_t length, int prot, int flags,
			      int fd, off_t pgoffset);
typedef void *mremap_function (void *old_address, size_t old_length,
			       size_t new_length, int flags, ...);
typedef void *realloc_function (void *addr, size_t size);

/* Declares the libc version of the functions we hook */
extern calloc_function __libc_calloc;
extern malloc_function __libc_malloc;
extern realloc_function __libc_realloc;
extern void __libc_free (void *ptr);

/* Directly calls the mmap() syscall */
static void *
kernel_mmap (void *start, size_t length, int prot, int flags,
	     int fd, off_t offset)
{
#ifdef __i386__
  int32_t args[6] =
    { (int32_t) start, (int32_t) length, prot, flags, fd, offset };
  return (void *) syscall (SYS_mmap, args);
#else
  return (void *) syscall (SYS_mmap, start, length, prot, flags, fd, offset);
#endif
}

/* Directly calls the mmap2() syscall */
static void *
kernel_mmap2 (void *start, size_t length, int prot, int flags,
	      int fd, off_t pgoffset)
{
  assert (MMAP2_ENABLED);
#if MMAP2_ENABLED
  return (void *) syscall (SYS_mmap2, start, length, prot, flags, fd,
			   pgoffset);
#else
  /* If MMAP2_ENABLED is not defined, we won't export mmap2, so this
   * function should not be called.
   */
  error (1, 0, "ksm_preload: Fatal error: mmap2 was called but not"
	 " exported. Please contact unbrice@vleu.net .");
  return NULL;
#endif
}

/* This structure contains all global variables. */
static struct
{
  /* The functions that the program would be using if we weren't preloaded.
   * Temporarily set to "safe" values during initialisation
   */
  calloc_function *ext_calloc;
  malloc_function *ext_malloc;
  mmap_function *ext_mmap;
  mmap2_function *ext_mmap2;
  mremap_function *ext_mremap;
  realloc_function *ext_realloc;
  /* The page size, this value is temporary and will be fixed
   * by setup()
   */
  unsigned long page_size;
  /* Zones smaller than this won't be merged */
  int merge_threshold;
} globals =
{
#if __GLIBC_PREREQ(2,12) || KSMP_FORCE_LIBC
  __libc_calloc,	// libc calloc
  __libc_malloc,	// libc malloc
  kernel_mmap,		// calls kernel mmap
  kernel_mmap2,		// calls mmap2 mmap
  NULL,			// mremap, unused during initialisation
  __libc_realloc,	// libc realloc
#else
# error This version of ksm_preload has not been tested with your	\
  libC. Please define KSMP_FORCE_LIBC to 1 (-DKSMP_FORCE_LIBC=1) and	\
  tell me about the result.
#endif
  4096,				// page_size
  DEFAULT_MERGE_THRESHOLD	//merge_threshold
};



/******** SETUP ********/

/* puts()-like, only enabled in DEBUG mode  */
static void
debug_puts (const char *str)
{
#ifdef DEBUG
  fprintf (stderr, "ksm_preload: %s\n", str);
#else
  (void) str;			// disables a warning about unused str
#endif
}

/* printf()-like, only enabled in DEBUG mode  */
static void
debug_printf (const char *fmt, ...)
{
#ifdef DEBUG
  char *strp;
  va_list var_args;
  va_start (var_args, fmt);
  if (vasprintf (&strp, fmt, var_args) > 0)
    debug_puts (strp);
  else
    debug_puts ("debug_printf failed");
  va_end (var_args);
#else
  (void) fmt;			// disables a warning about unused fmt
#endif
}

/* Gets an environment variable from its name and parses it as a
 * positive integer.
 * Returns the parsed value truncated to INT_MAX, -1 if undefined or invalid.
 */
static int
get_uint_from_environment (const char *var_name)
{
  char *var_string = getenv (var_name);
  char *var_string_end = var_string;
  long int var_value;		// var_string as a long itn

  if (NULL == var_string)
    return -1;
  else
    var_value = strtol (var_string, &var_string_end, 10);

  /* Validates strtol's return value */
  if (*var_string_end != '\0' || var_value < 0)
    {
      debug_printf ("Invalid environment variable %s=%s, a"
		    " positive integer was expected.");
      return -1;
    }
  else if (var_value > INT_MAX)
    {
      debug_printf ("Truncated %s to INT_MAX(%i) ", var_name, INT_MAX);
      return INT_MAX;
    }
  else
    return var_value;
}

/* Just like dlsym but error()s in case of failure */
static void *
xdlsym (void *handle, const char *symbol)
{
  void *res = dlsym (handle, symbol);
  if (res)
    return res;
  else
    {
      error (1, 0, "failed to load %s : %s", symbol, dlerror ());
      return NULL;
    }
}

/* Sets the globals.* variables */
static void
setup ()
{
  /* Loads the symbols from the next library using the libc functions
   * We will set them at once to avoid a situation where we would be
   * using some of them, and some of the default ones
   */
  calloc_function *dl_calloc = xdlsym (RTLD_NEXT, "calloc");
  malloc_function *dl_malloc = xdlsym (RTLD_NEXT, "malloc");
  mmap_function *dl_mmap = xdlsym (RTLD_NEXT, "mmap");
  mremap_function *dl_mremap = xdlsym (RTLD_NEXT, "mremap");
  realloc_function *dl_realloc = xdlsym (RTLD_NEXT, "realloc");

  /* Treats mmap2 as a special case because dlsym fails to find it on
   * Ubuntu 10.10, even when it is available.
   */
  // TODO(unbrice): investigate...
  if (MMAP2_ENABLED)
    {
      mmap2_function *dl_mmap2 = dlsym (RTLD_NEXT, "mmap2");
      if (NULL != dl_mmap2)
	globals.ext_mmap2 = dl_mmap2;
    }

  /* Get parameters from the environment */
  globals.page_size = (long unsigned) sysconf (_SC_PAGESIZE);
  globals.merge_threshold =
    get_uint_from_environment (MERGE_THRESHOLD_ENV_NAME);
  if (globals.merge_threshold == -1)
    globals.merge_threshold = DEFAULT_MERGE_THRESHOLD;

  /* Activates the symbols from the next library */
  globals.ext_calloc = dl_calloc;
  globals.ext_malloc = dl_malloc;
  globals.ext_mmap = dl_mmap;
  globals.ext_mremap = dl_mremap;
  globals.ext_realloc = dl_realloc;

  debug_puts ("Setup done.");
}


/******** UTILITIES FOR WRAPPERS ********/

static void
lazily_setup ()
{
  /* Allows to be notified when setup has returned */
  static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
  /* Protects the other variables */
  static pthread_mutex_t condition_mutex =
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
  /* True if setup() has been called and returned */
  static bool setup_done = false;
  /* True if setup() was called or is being called */
  static bool setup_started = false;
  /* The thread in charge for running setup(), invalid if
   * setup_started is false
   */
  static pthread_t setup_thread;

  /* Quickly returns if the job was already done */
  __sync_synchronize ();	// updates globals.* variables
  if (setup_done)
    return;

  // <condition_mutex>
  pthread_mutex_lock (&condition_mutex);
  if (!setup_done)		// Might have been called since last check
    {
      if (!setup_started)
	{
	  /* Setup hasn't been called yet, start it */
	  setup_thread = pthread_self ();
	  setup_started = true;
	  setup ();
	  setup_done = true;
	  pthread_cond_broadcast (&condition);
	}
      else			/* if (setup_started) */
	{
	  if (pthread_self () != setup_thread)
	    /* Another thread is doing the setup, wait for it */
	    pthread_cond_wait (&condition, &condition_mutex);
	  else
	    {
	      /* We are the ones doing the setup ! So we are called from setup,
	       * because it is allocating memory. Nothing to do.
	       */
	    }
	}
    }
  // </condition_mutex>
  pthread_mutex_unlock (&condition_mutex);
}

/* Issues a madvise(..., MADV_MERGEABLE) if len is big enough and flags are rights.
 * Flags are ignores if flags == -1
 */
static void
merge_if_profitable (void *address, size_t length, int flags)
{

  /* Rounds address to its page */
  const uintptr_t raw_address = (uintptr_t) address;
  const uintptr_t page_address =
    (raw_address / globals.page_size) * globals.page_size;
  assert (page_address <= raw_address);

  /* Computes the new length */
  const size_t new_length = length + (size_t) (raw_address - page_address);

  if (new_length <= globals.merge_threshold || NULL == address)
    return;
  /* Checks that required flags are present and that forbidden ones are not */
  else if (flags == -1		// flags are unknown
	   // Checks for required flags, avoids the stacks
	   || ((flags & MAP_PRIVATE) && (flags & MAP_ANONYMOUS)
	       && !(flags & MAP_GROWSDOWN) && !(flags & MAP_STACK)))
    {
      if (-1 == madvise ((void *) page_address, new_length, MADV_MERGEABLE))
	debug_puts ("madvise() failed");
      else
	debug_puts ("Sharing");
    }
  else
    debug_puts ("Not sharing (flags filtered)");
}


/******** WRAPPERS ********/

/* Just like calloc() but calls merge_if_profitable */
void *
calloc (size_t nmemb, size_t size)
{
  lazily_setup ();
  void *res = globals.ext_calloc (nmemb, size);
  merge_if_profitable (res, size, -1);
  return res;
}

/* Just like malloc() but calls merge_if_profitable */
void *
malloc (size_t size)
{
  lazily_setup ();
  void *res = globals.ext_malloc (size);
  merge_if_profitable (res, size, -1);
  return res;
}

/* Just like mmap() but calls merge_if_profitable */
void *
mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
  lazily_setup ();
  void *res = globals.ext_mmap (addr, length, prot, flags, fd, offset);
  merge_if_profitable (res, length, flags);
  return res;
}

#if ! MMAP2_ENABLED
static				// Disables mmap2
#endif
/* Just like mmap2() but calls merge_if_profitable */
void *
mmap2 (void *addr, size_t length, int prot, int flags, int fd, off_t pgoffset)
{
  assert (MMAP2_ENABLED);
  lazily_setup ();
  void *res = globals.ext_mmap2 (addr, length, prot, flags, fd, pgoffset);
  merge_if_profitable (res, length, flags);
  return res;
}

/* Just like mremap() but calls merge_if_profitable */
void *
mremap (void *old_address, size_t old_length, size_t new_length, int flags,
	...)
{
  lazily_setup ();
  void *res;
  if (flags & MREMAP_FIXED)
    {
      /* This is the five-arguments version of mremap. */
      // It sometimes happens that the kernel's API is so ugly…
      void *target_address;
      va_list extra_args;
      va_start (extra_args, flags);
      target_address = va_arg (extra_args, void *);
      va_end (extra_args);
      res = globals.ext_mremap (old_address, old_length, new_length, flags,
				target_address);
    }
  else
    res = globals.ext_mremap (old_address, old_length, new_length, flags);
  merge_if_profitable (res, new_length, -1);
  return res;
}

/* Just like realloc() but calls merge_if_profitable */
void *
realloc (void *addr, size_t size)
{
  lazily_setup ();
  void *res = globals.ext_realloc (addr, size);
  merge_if_profitable (res, size, -1);
  return res;
}
