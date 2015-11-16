#ifndef BITCOIN_CORPUS_H
#define BITCOIN_CORPUS_H
#include <ccan/short_types/short_types.h>
#include <ccan/endian/endian.h>
#include <string.h>

struct corpus_txid {
	u8 id[32];
};

enum corpus_entry_type {
	INCOMING_TX = 1,
	COINBASE = 2,
	UNKNOWN = 3,
	KNOWN = 4,
	MEMPOOL_ONLY = 5
};

struct corpus_entry {
	/* Seconds since 1970 UTC */
	le32 timestamp;
	/* Lower 8 bit type (above) and upper 24 bit block number */
	le32 type_and_blocknum;
	/* Double-SHA of transaction */
	struct corpus_txid txid;
};

static inline enum corpus_entry_type corpus_entry_type(const struct corpus_entry *e)
{
	return (enum corpus_entry_type)(le32_to_cpu(e->type_and_blocknum) & 0xFF);
}

static inline unsigned int corpus_blocknum(const struct corpus_entry *e)
{
	return le32_to_cpu(e->type_and_blocknum) >> 8;
}

/* These block heights have orphans. */
static inline bool corpus_maybe_orphan(size_t blocknum)
{
	return blocknum == 352802 || blocknum == 352548 || blocknum == 352560
		|| blocknum == 353014;
}

/* We identify orphaned blocks by coinbase. */
static inline bool corpus_orphaned_coinbase(const struct corpus_entry *e)
{
	static struct corpus_txid orphan_352802
		= { { 0x79, 0xb1, 0xc3, 0x09, 0xab, 0x8a, 0xb9, 0x2b,
		      0xca, 0x4d, 0x07, 0x50, 0x8e, 0x0f, 0x59, 0x6f,
		      0x87, 0x2f, 0x66, 0xc6, 0xdb, 0x4d, 0x36, 0x67,
		      0x13, 0x3a, 0x37, 0x17, 0x20, 0x55, 0xe9, 0x7b } };
	static struct corpus_txid orphan_352548
		= { { 0xa0, 0x1b, 0x5e, 0x45, 0xd3, 0x62, 0x4b, 0xc0,
		      0x26, 0x5f, 0xb8, 0xab, 0x81, 0xbb, 0x99, 0x6b,
		      0xf4, 0xff, 0xd4, 0x6d, 0xdd, 0xe4, 0x5e, 0x08,
		      0x3f, 0xc7, 0x3e, 0x33, 0x4e, 0x77, 0x6e, 0x0d } };
	static struct corpus_txid orphan_352560
		= { { 0x33, 0xdb, 0x97, 0x55, 0x66, 0x2f, 0x6b, 0x4a,
		      0x46, 0xdf, 0xe2, 0x6a, 0x1d, 0x65, 0xba, 0x00,
		      0xc4, 0xe1, 0xa2, 0xa9, 0xf1, 0xdb, 0x19, 0x0e,
		      0x71, 0x1c, 0x61, 0xe4, 0xbc, 0xd0, 0x60, 0xd7 } };
	static struct corpus_txid orphan_353014
		= { { 0x83, 0x01, 0x78, 0xaa, 0x3b, 0x5b, 0xff, 0xfa,
		      0x0d, 0x8e, 0x2d, 0xc3, 0x9d, 0xef, 0x5d, 0x3e,
		      0x99, 0x02, 0x9c, 0xa5, 0x0a, 0xaf, 0xc5, 0xc1,
		      0x72, 0xe4, 0x55, 0x6f, 0x9a, 0xc4, 0x6d, 0x1e } };

	if (corpus_entry_type(e) != COINBASE)
		return false;

	if (!corpus_maybe_orphan(corpus_blocknum(e)))
		return false;

	return memcmp(&orphan_352802, &e->txid, sizeof(e->txid)) == 0
		|| memcmp(&orphan_352548, &e->txid, sizeof(e->txid)) == 0
		|| memcmp(&orphan_352560, &e->txid, sizeof(e->txid)) == 0
		|| memcmp(&orphan_353014, &e->txid, sizeof(e->txid)) == 0;
}
#endif /* BITCOIN_CORPUS_H */
