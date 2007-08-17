/* public cdb include file
 *
 * This file is a part of tinycdb package by Michael Tokarev, mjt@corpit.ru.
 * Public domain.
 */
#include <cstdio>

#ifndef TINYCDB_VERSION
#define TINYCDB_VERSION 0.74

namespace pvpgn
{

typedef unsigned int cdbi_t; /* compatibility */

/* common routines */
unsigned cdb_hash(const void *buf, unsigned len);
unsigned cdb_unpack(const unsigned char buf[4]);
void cdb_pack(unsigned num, unsigned char buf[4]);

struct cdb {
  std::FILE *cdb_fd;			/* file descriptor */
  /* private members */
  unsigned cdb_fsize;		/* datafile size */
  unsigned cdb_dend;		/* end of data ptr */
  const unsigned char *cdb_mem; /* mmap'ed file memory */
  unsigned cdb_vpos, cdb_vlen;	/* found data */
  unsigned cdb_kpos, cdb_klen;	/* found key */
};

#define CDB_STATIC_INIT {0,0,0,0,0,0,0,0}

#define cdb_datapos(c) ((c)->cdb_vpos)
#define cdb_datalen(c) ((c)->cdb_vlen)
#define cdb_keypos(c) ((c)->cdb_kpos)
#define cdb_keylen(c) ((c)->cdb_klen)
#define cdb_fileno(c) ((c)->cdb_fd)

int cdb_init(struct cdb *cdbp, std::FILE *fd);
void cdb_free(struct cdb *cdbp);

int cdb_read(const struct cdb *cdbp,
	     void *buf, unsigned len, unsigned pos);
#define cdb_readdata(cdbp, buf) \
	cdb_read((cdbp), (buf), cdb_datalen(cdbp), cdb_datapos(cdbp))
#define cdb_readkey(cdbp, buf) \
	cdb_read((cdbp), (buf), cdb_keylen(cdbp), cdb_keypos(cdbp))

const void *cdb_get(const struct cdb *cdbp, unsigned len, unsigned pos);
#define cdb_getdata(cdbp) \
	cdb_get((cdbp), cdb_datalen(cdbp), cdb_datapos(cdbp))
#define cdb_getkey(cdbp) \
	cdb_get((cdbp), cdb_keylen(cdbp), cdb_keypos(cdbp))

int cdb_find(struct cdb *cdbp, const void *key, unsigned klen);

struct cdb_find {
  struct cdb *cdb_cdbp;
  unsigned cdb_hval;
  const unsigned char *cdb_htp, *cdb_htab, *cdb_htend;
  unsigned cdb_httodo;
  const void *cdb_key;
  unsigned cdb_klen;
};

int cdb_findinit(struct cdb_find *cdbfp, struct cdb *cdbp,
		 const void *key, unsigned klen);
int cdb_findnext(struct cdb_find *cdbfp);

#define cdb_seqinit(cptr, cdbp) ((*(cptr))=2048)
int cdb_seqnext(unsigned *cptr, struct cdb *cdbp);

/* old simple interface */
/* open file using standard routine, then: */
int cdb_seek(std::FILE *fd, const void *key, unsigned klen, unsigned *dlenp);
int cdb_bread(std::FILE *fd, void *buf, int len);

/* cdb_make */

struct cdb_make {
  std::FILE *cdb_fd;			/* file descriptor */
  /* private */
  unsigned cdb_dpos;		/* data position so far */
  unsigned cdb_rcnt;		/* record count so far */
  char cdb_buf[4096];		/* write buffer */
  char *cdb_bpos;		/* current buf position */
  struct cdb_rl *cdb_rec[256];	/* list of arrays of record infos */
};

int cdb_make_start(struct cdb_make *cdbmp, std::FILE *fd);
int cdb_make_add(struct cdb_make *cdbmp,
		 const void *key, unsigned klen,
		 const void *val, unsigned vlen);
int cdb_make_exists(struct cdb_make *cdbmp,
		    const void *key, unsigned klen);
int cdb_make_put(struct cdb_make *cdbmp,
		 const void *key, unsigned klen,
		 const void *val, unsigned vlen,
		 int flag);
#define CDB_PUT_ADD	0	/* add unconditionnaly, like cdb_make_add() */
#define CDB_PUT_REPLACE	1	/* replace: do not place to index OLD record */
#define CDB_PUT_INSERT	2	/* add only if not already exists */
#define CDB_PUT_WARN	3	/* add unconditionally but ret. 1 if exists */
int cdb_make_finish(struct cdb_make *cdbmp);

}

#endif /* include guard */
