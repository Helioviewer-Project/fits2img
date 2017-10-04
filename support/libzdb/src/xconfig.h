/* src/xconfig.h.  Generated from xconfig.h.in by configure.  */
/* src/xconfig.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `crypto' library (-lcrypto). */
/* #undef HAVE_LIBCRYPTO */

/* Define to 1 to enable mysql */
#define HAVE_LIBMYSQLCLIENT 1

/* Define to 1 to enable postgresql */
#define HAVE_LIBPQ 1

/* Define to 1 if you have the <libpq-fe.h> header file. */
#define HAVE_LIBPQ_FE_H 1

/* Define to 1 to enable sqlite3 */
#define HAVE_LIBSQLITE3 1

/* Define to 1 if you have the `ssl' library (-lssl). */
/* #undef HAVE_LIBSSL */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <mysql.h> header file. */
#define HAVE_MYSQL_H 1

/* Define to 1 to enable oracle */
/* #undef HAVE_ORACLE */

/* sqlite3_errstr */
#define HAVE_SQLITE3_ERRSTR 1

/* sqlite3_soft_heap_limit */
#define HAVE_SQLITE3_SOFT_HEAP_LIMIT 1

/* sqlite3_soft_heap_limit64 */
#define HAVE_SQLITE3_SOFT_HEAP_LIMIT64 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if `tm_gmtoff' is a member of `struct tm'. */
#define HAVE_STRUCT_TM_TM_GMTOFF 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the `timegm' function. */
#define HAVE_TIMEGM 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "libzdb"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "bugs-libzdb@tildeslash.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libzdb"

/* Define to 1 to package protect (hide) non-api objects */
/* #undef PACKAGE_PROTECTED */

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libzdb 3.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libzdb"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.1"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "3.1"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Define to 1 to hide objects for linking with zild */
/* #undef ZILD_PACKAGE_PROTECTED */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */
