/* mdb_from_db.c - translate Berkeley DB to memory-mapped database(s) */
/*
 * Copyright 2014 Jonathan Graehl, 2011-2014 Howard Chu, Symas Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "lmdb.h"
#include <db.h>
#include <stdbool.h>
#include <sys/stat.h>

static int batchsize = 10000;
static size_t const MB = 1024 * 1024;
static int datasize = 64 * 1024 * 1024;
static unsigned mapMB = 1024 * 1024; // so 1TB default limit
inline size_t mapBytes() {
  return (size_t)mapMB * (size_t)MB;
}

static char* subname = NULL;

static char* prog;

static MDB_val kbuf, dbuf;

#ifdef _WIN32
#define Z "I"
#else
#define Z "z"
#endif


char *usagestr =
    "path.input.berkeley.db [path.output.mdb|T-txt] [-P dbpasswd] [-V] [-l] [-n] [-N] [-s subdbname] [-H homedirpath] [-b write-batchsize] [-m map_megabytes] [-o redirect_stdout.txt] [-B] [-T] [-v] [-d] [-a] [-1] [-C] [-x] [-F]\n\n"
    " (-m MB: limit database size to this many megabytes - use as large a value as your system supports (short of 4 billion)\n"
    " (-n: create single mdb file instead of dir - the default)\n"
    " (-N: create a directory with a file for each subdb)\n"
    " (-f: drop the old database entirely (removing all records) before adding the ones from the input db - safer than simply rm -rf path.output.mdb first since it will correctly  fail if db is i)\n"
    " (-T prints to stdout key/val in mdb_load format)\n"
    " (-l: only list (to stdout) database names)\n"
    " (-B nonblocking db open)\n"
    " (-1: only one data per key allowed - do not set MDB_DUPSORT)\n"
    " (-x: don't add if key already exists in any case (even if MDB_DUPSORT))\n"
    " (-d: don't add duplicate <key, data> pairs (but do allow different data w/ same key e.g. <key, otherdata>)\n"
    " (-H dir: ('home' dir for relative db filenames default '.')\n"
    " (-o stdout_file: write -T stdout here instead)\n"
    " (-V: print version and exit)\n"
    " (-b N: batch size=N - default 100)\n"
    " (-a: append assuming bdb has same sort order as mdb (error if db not sorted same as mdb))\n"
    " (-C: don't use cursor put (might be slower - uses random access write requests)\n"
    " (-F: sort keys left->right (default is right->left, i.e. sorting on reversed strings)\n"
    " (-R: sort keys right-left (the default)\n"
    " (on map-full or txn-full errors, try again with larger -m map_megabytes e.g. -m 4096 is 4GB)\n"
    ;

/**
   fail() and shutdown(): everything that might need to be cleaned up on error->exit
   (conceptually some of these are locals, but having them global lets us call
   fail() from anywhere)
*/
static MDB_env* env;
static MDB_txn* txn;
static MDB_dbi dbi;

static DB* dbp;
static DBC* dbcp;
static DB* parent_dbp;
static DBC* bdb_subdbcursor;
static char* subdbname;
static DB_TXN* dbtxn;

void bdb_close() {
  if (dbcp) dbcp->close(dbcp);
  dbcp = 0;
  if (dbp) dbp->close(dbp, 0);
  dbp = 0;
}
void shutdown() {
  if (bdb_subdbcursor) bdb_subdbcursor->close(bdb_subdbcursor);
  bdb_subdbcursor = 0;
  bdb_close();
  if (parent_dbp) parent_dbp->close(parent_dbp, 0);
  parent_dbp = 0;
  if (txn) mdb_txn_abort(txn);
  txn = 0;
  if (dbi) mdb_dbi_close(env, dbi);
  if (env) mdb_env_close(env);
  env = 0;
  if (subdbname) free(subdbname);
  subdbname = 0;
}

void fail() {
  shutdown();
  exit(EXIT_FAILURE);
}

static void usage(void) {
  fprintf(stderr, "usage: %s %s", prog, usagestr);
  fail();
}

/**
   BDB env
*/
static char* dbhome;
static DB_ENV* dbenv;
static u_int32_t dbcache = 1024 * 1024;
static char* dbpasswd;
static bool dbnonblocking;
void strfill(char* str, char fill) {
  while (*str) *str++ = fill;
}
void bdb_err(char* fn, int rc) {
  fprintf(stderr, "%s: ", prog);
  if (dbenv)
    dbenv->err(dbenv, rc, fn);
  else
    fprintf(stderr, "%s\n", db_strerror(rc));
  fail();
}

DBT dbkey, dbdata;

/**
   BDB db.
*/
static char* dbfilename;
static DBT keyret, dataret;
static bool bdb_is_recno;
static db_recno_t bdb_recno;
static DB_HEAP_RID bdb_heaprid;
static int bdb_get_flags;
static int bdb_set_flags;
static void* pointer_get;

void bdb_init_dbenv() {
  int rc;
  if ((rc = db_env_create(&dbenv, 0)) != 0) bdb_err("db_env_create", rc);
  dbenv->set_errfile(dbenv, stderr);
  dbenv->set_errpfx(dbenv, prog);
  if (dbpasswd != NULL) {
    rc = dbenv->set_encrypt(dbenv, dbpasswd, DB_ENCRYPT_AES);
    bdb_set_flags |= DB_ENCRYPT;
    strfill(dbpasswd, '\0'); /* silly precaution - doesn't even hide password length */
    if (rc) bdb_err("dbenv::set_encrypt", rc);
  }
  if (dbnonblocking) {
    if ((rc = dbenv->set_flags(dbenv, DB_NOLOCKING, 1))) bdb_err("DB_NOLOCKING", rc);
    if ((rc = dbenv->set_flags(dbenv, DB_NOPANIC, 1))) bdb_err("DB_NOPANIC", rc);
    if ((rc = dbenv->set_flags(dbenv, DB_TXN_NOSYNC, 1))) bdb_err("DB_TXN_NOSYNC", rc);
  }
  if ((rc = dbenv->set_cachesize(dbenv, 0, dbcache, 1))) bdb_err("dbenv::set_cachesize", rc);
  if ((rc = dbenv->open(dbenv, dbhome, DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE | DB_USE_ENVIRON, 0)))
    bdb_err("dbenv::open", rc);
  dbdata.flags = DB_DBT_USERMEM;
  if (!(dbdata.data = malloc(dbdata.ulen = dbcache))) fail();
}

void bdb_open(char* dbname) {
  int rc;
  bdb_close();
  if ((rc = db_create(&dbp, dbenv, 0))) bdb_err("db_create", rc);
  if ((rc = dbp->set_flags(dbp, bdb_set_flags))) bdb_err("db_set", rc);
  if ((rc = dbp->open(dbp, dbtxn, dbfilename, dbname, DB_UNKNOWN,
                      (parent_dbp ? 0 : DB_RDWRMASTER) | DB_RDONLY, 0))) {
    fprintf(stderr, "db open %s : %s\n", dbfilename, dbname);
    bdb_err(dbfilename, rc);
  }
}

void bdb_start_chunks() {
  int rc;
  bdb_get_flags = DB_NEXT | DB_MULTIPLE_KEY;
  if ((bdb_is_recno = (dbp->type == DB_RECNO || dbp->type == DB_QUEUE))) {
    keyret.data = &bdb_recno;
    keyret.size = sizeof(bdb_recno);
  } else if (dbp->type == DB_HEAP) {
    bdb_get_flags = DB_NEXT;
    dbkey.flags = DB_DBT_USERMEM;
    dbkey.data = &bdb_heaprid;
    dbkey.size = dbkey.ulen = sizeof(bdb_heaprid);
  }
  if ((rc = dbp->cursor(dbp, NULL, &dbcp, 0))) bdb_err("cursor", rc);
}

unsigned align(unsigned req, unsigned granule) {
  return ((req + granule - 1) / granule) * granule;
}

/**
   \return true if there's another chunk of records.
*/
bool bdb_read_chunk() {
  int rc;
  if ((rc = dbcp->get(dbcp, &dbkey, &dbdata, bdb_get_flags))) {
    if (rc == DB_NOTFOUND) return false;
    if (rc == DB_BUFFER_SMALL) {
      dbdata.ulen = dbdata.size = align(dbdata.size, 4096);
      if (!(dbdata.data = realloc(dbdata.data, dbdata.size))) fail();
      rc = dbcp->get(dbcp, &dbkey, &dbdata, bdb_get_flags);
    }
    if (rc) bdb_err("get chunk", rc);
  }
  DB_MULTIPLE_INIT(pointer_get, &dbdata);
  return true;
}

/**
   \return true if there was another record; sets keyret and dataret.
*/
bool bdb_next_record_in_chunk() {
  if (bdb_is_recno)
    DB_MULTIPLE_RECNO_NEXT(pointer_get, &dbdata, bdb_recno, dataret.data, dataret.size);
  else
    DB_MULTIPLE_KEY_NEXT(pointer_get, &dbdata, keyret.data, keyret.size, dataret.data, dataret.size);
  return dataret.data;
}

static char hexc_[] = "0123456789ABCDEF";

char hexc(unsigned char i) {
  return hexc_[i];
}

void putchar_T(unsigned char c) {
  if (c >= 32 && c < 127 && c != '\\') { putchar(c); } else {
    putchar('\\');
    putchar(hexc(c >> 4));
    putchar(hexc(c & 0xf));
  }
}

void putchar_T_to(unsigned char c, FILE* to) {
  if (c >= 32 && c < 127 && c != '\\') { fputc(c, to); } else {
    fputc('\\', to);
    fputc(hexc(c >> 4), to);
    fputc(hexc(c & 0xf), to);
  }
}

/**
   TODO: could fwrite chunks of no-escape-needed bytes, or probably faster,
   encode in memory then write once
*/
void print_T(char* data, unsigned len) {
  unsigned i = 0;
  for (; i < len; ++i) putchar_T(data[i]);
}

void print_T_to(char* data, unsigned len, FILE* to) {
  unsigned i = 0;
  for (; i < len; ++i) putchar_T_to(data[i], to);
}

/**
   Paired lines of text, where the first line of the pair is the key item, and the
   second line of the pair is its corresponding data item.

   A simple escape mechanism, where newline and backslash (\\) characters are special, is
   applied to the text input. Newline characters are interpreted as record separators.
   Backslash characters in the text will be interpreted in one of two ways: If the backslash
   character precedes another backslash character, the pair will be interpreted as a literal
   backslash. If the backslash character precedes any other character, the two characters
   following the backslash will be interpreted as a hexadecimal specification of a single
   character; for example, \\0a is a newline character in the ASCII character set.

   For this reason, any backslash or newline characters that naturally occur in the text
   input must be escaped to avoid misinterpretation by
*/
void print_record_T() {
  print_T(keyret.data, keyret.size);
  putchar('\n');
  print_T(dataret.data, dataret.size);
  putchar('\n');
}


char* bdb_open_subdb(DBT key) {
  if (!(subdbname = malloc(key.size + 1))) fail();
  memcpy(subdbname, key.data, key.size);
  subdbname[key.size] = '\0';
  bdb_open(subdbname);
  return subdbname;
}

bool isdir(char* path) {
  struct stat s;
  if (stat(path, &s)) {
    perror("path");
    fail();
  }
  return S_ISDIR(s.st_mode);
}

void mkdir_if_needed(char* path) {
  if (mkdir(path, 0755))
    if (errno != EEXIST) {
      perror(path);
      fail();
    }
  if (!isdir(path)) {
    fprintf(stderr,
            "%s is not a directory and can't mkdir it. try with -n for no-subdir (to store as a file)",
            path);
    fail();
  }
}

int main(int argc, char* argv[]) {
  int i, rc;
  MDB_cursor* cursor;
  int envflags = 0, putflags = 0, openflags = MDB_CREATE;
  int textflag = false;
  bool havemultiple;
  int maxdbs = 2; /* parent db-of-dbs plus current output db */

  prog = argv[0];

  if (argc < 2) { usage(); }

  bool dupdata = false;
  bool subdir = false;
  bool dupsort = true;
  bool listdbs = false;
  bool appendsorted = false;
  bool overwrite = false;
  bool cursorput = true; /* should be faster than individual put */
  bool force = false;
  bool reversekey = true;
  while ((i = getopt(argc, argv, "o:P:M:m:H:s:b:lnvVTNS1xacCfFNRFh?")) != EOF) {
    switch (i) {
      case 'm':
      case 'M':
        i = sscanf(optarg, "%du", &mapMB);
        if (i != 1) {
          fprintf(stderr, "ERROR: -M '%s' (batchsize) was not size_t\n", optarg);
          usage();
        }
        break;
      case 'b':
        i = sscanf(optarg, "%d", &batchsize);
        if (i != 1) {
          fprintf(stderr, "ERROR: -b '%s' (batchsize) was not int\n", optarg);
          usage();
        }
        break;
      case 'o':
        if (freopen(optarg, "w", stdout) == NULL) {
          fprintf(stderr, "%s: %s: reopen: %s\n", prog, optarg, strerror(errno));
          exit(EXIT_FAILURE);
        }
        break;
      case 'v':
      case 'V':
        printf("%s\n", MDB_VERSION_STRING);
        printf("%s\n", db_version(NULL, NULL, NULL));
        if (i == 'V') exit(0);
        break;
      case 'S':
        subdir = true;
        break;
      case 'N':
        subdir = true;
        break;
      case 'n':
        subdir = false;
        break;
      case 's':
        subname = strdup(optarg);
        break;
      case 'd':
        dupdata = true;
        break;
      case '1':
        dupsort = false;
        break;
      case 'a':
        appendsorted = true;
        break;
      case 'x':
        overwrite = true;
        break;
      case 'B':
        dbnonblocking = true;
        break;
      case 'T':
        textflag = true;
        break;
      case 'H':
        dbhome = optarg;
        break;
      case 'C':
        cursorput = false;
        break;
      case 'c':
        cursorput = true;
        break;
      case 'P':
        /**
           we XXX password immediately on init, to hide from top etc. but would
           be better to get from stdin (XXX earlier would still be insecure)
        */
        dbpasswd = optarg;
        break;
      case 'l':
        listdbs = true;
        break;
      case 'f':
        force = true;
        break;
      case 'R':
        reversekey = true;
        break;
      case 'F':
        reversekey = false;
        break;
      case '?':
      case 'h':
      default:
        usage();
    }
  }

  if (overwrite) putflags |= MDB_NOOVERWRITE;

  if (dupdata) putflags |= MDB_NODUPDATA;

  if (!subdir) envflags |= MDB_NOSUBDIR;

  if (appendsorted) putflags |= (dupsort ? MDB_APPENDDUP : MDB_APPEND);

  if (dupsort) openflags |= MDB_DUPSORT;

  if (reversekey) openflags |= MDB_REVERSEKEY;

  bool haveout = optind == argc - 2;
  if (optind >= argc) usage();
  dbfilename = argv[optind++];
  char* mdboutpath = haveout ? argv[optind++] : NULL;
  if (mdboutpath) {
    if (subdir) mkdir_if_needed(mdboutpath);
  }
  if (listdbs) {
    if (textflag)
      fprintf(stderr, "disabling -T (print key/val lines) because -l (list dbs) was specified\n");
    textflag = false;
  }
  fprintf(stderr, "prepared: dupsort=%d reversekey=%d putflags=%x openflags=%x envflags=%x cursor=%d\n",
          (int)dupsort, (int)reversekey, putflags, openflags, envflags, (int)cursorput);

  /**
     args parsed.

     init BDB:
  */
  bdb_init_dbenv();
  bdb_open(subname);

/**
   init MDB:
*/
#undef MDB_OK
#define MDB_OK(call)                                                        \
  if (rc) {                                                                 \
    fprintf(stderr, #call " failed - error %d %s\n", rc, mdb_strerror(rc)); \
    goto shutdown;                                                          \
  } else {}

  if (mdboutpath) {
    rc = mdb_env_create(&env);
    MDB_OK(mdb_env_create);

    size_t mapbytes = mapBytes();
    rc = mdb_env_set_mapsize(env, mapbytes);
    fprintf(stderr, "set mdb max (map) size to %u MB = %.3f GB\n", mapMB, (double)mapbytes/(1024.*1024.*1024.));
    MDB_OK(mdb_env_set_mapsize);

    rc = mdb_env_set_maxdbs(env, maxdbs);
    MDB_OK(mdb_env_set_maxdbs);

    rc = mdb_env_open(env, mdboutpath, envflags, 0664);
    MDB_OK(mdb_env_open);

    kbuf.mv_size = mdb_env_get_maxkeysize(env) * 2 + 2;
    kbuf.mv_data = malloc(kbuf.mv_size);

    dbuf.mv_size = datasize;
    dbuf.mv_data = malloc(dbuf.mv_size);
  }

  havemultiple = !subname && dbp->get_multiple(dbp);
  if (havemultiple) {
    parent_dbp = dbp;
    dbp = 0;
    if ((rc = parent_dbp->cursor(parent_dbp, NULL, &bdb_subdbcursor, 0))) bdb_err("cursor(sub-dbs)", rc);
  }

  unsigned long long wnrecords, wnrecordsall = 0;
  unsigned long long nrecords, nrecordsall = 0;
  unsigned ndbs = 0;
  bool const reading = textflag || mdboutpath;
  for (;;) {
    MDB_val key, data;
    int batch = 0;
    if (havemultiple) {
      if ((rc = bdb_subdbcursor->get(bdb_subdbcursor, &dbkey, &dbdata, DB_NEXT | DB_IGNORE_LEASE))) {
        if (rc != DB_NOTFOUND)
          bdb_err("get-next-subdb", rc);
        else
          rc = 0;
        break;
      }
      subname = bdb_open_subdb(dbkey);
    }

    ++ndbs;
    nrecords = 0;
    wnrecords = 0;
    if (subname) {
      if (listdbs) printf("%s\n", subname);
      if (reading) fprintf(stderr, "reading DB %s ", subname);
    } else {
      listdbs = false;
      fprintf(stderr, "reading unnamed DB ");
    }

    if (bdb_is_recno) openflags |= MDB_INTEGERKEY;

    if (mdboutpath) {
      rc = mdb_txn_begin(env, NULL, 0, &txn);
      MDB_OK(mdb_txn_begin);
      rc = mdb_dbi_open(txn, subname,
                        (bdb_is_recno ? MDB_INTEGERKEY : 0) | MDB_CREATE | (dupsort ? MDB_DUPSORT : 0),
                        &dbi);
      MDB_OK(mdb_open);
      if (force) {
        rc = mdb_drop(txn, dbi, 0);
        MDB_OK(mdb_drop);
      }
      if (mdboutpath && cursorput) {
        rc = mdb_cursor_open(txn, dbi, &cursor);
        MDB_OK(mdb_cursor_open);
      }
    }

    if (reading) {
      bdb_start_chunks();
      while (bdb_read_chunk()) {
        while (bdb_next_record_in_chunk()) {
          ++nrecords;
          if (textflag) print_record_T();
          if (mdboutpath) {
            key.mv_data = keyret.data;
            key.mv_size = keyret.size;
            data.mv_data = dataret.data;
            data.mv_size = dataret.size;
            if (cursorput)
              rc = mdb_cursor_put(cursor, &key, &data, putflags);
            else
              rc = mdb_put(txn, dbi, &key, &data, putflags);
            if (rc == MDB_KEYEXIST) {
              fprintf(stderr, "duplicate encountered for key ");
              print_T_to(key.mv_data, key.mv_size, stderr);
              fprintf(stderr, "\n");
              if (dupdata | overwrite) { continue; } else
                fprintf(stderr, "assuming put->dupsort keeps duplicate");
            }
            ++wnrecords;
            MDB_OK(mdb_put);
            if (++batch == batchsize) {
              fputc('.', stderr);
              batch = 0;
              rc = mdb_txn_commit(txn);
              MDB_OK(mdb_txn_commit);
              rc = mdb_txn_begin(env, NULL, 0, &txn);
              MDB_OK(mdb_txn_begin);
              if (cursorput) {
                rc = mdb_cursor_open(txn, dbi, &cursor);
                MDB_OK(mdb_cursor_open);
              }
            }
          }
        }
      }
      if (mdboutpath) {
        rc = mdb_txn_commit(txn);
        txn = 0;
        MDB_OK(mdb_txn_commit);
        mdb_dbi_close(env, dbi);
        dbi = 0;
      }
      nrecordsall += nrecords;
      wnrecordsall += wnrecords;
      fprintf(stderr, " %llu records (stored %llu).\n", nrecords, wnrecords);
    }
    if (!havemultiple) break;
  }
  fprintf(stderr, "Found %u Berkeley DB(s) in input file %s - read %llu records", ndbs, dbfilename,
          nrecordsall);
  if (mdboutpath) fprintf(stderr, " (stored %llu to MDB %s).\n", wnrecordsall, mdboutpath);
  fprintf(stderr, "\n");
shutdown:
  shutdown();
  return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
