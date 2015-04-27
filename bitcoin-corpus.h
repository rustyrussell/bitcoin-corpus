#ifndef BITCOIN_CORPUS_H
#define BITCOIN_CORPUS_H
#include <ccan/short_types/short_types.h>
#include <ccan/endian/endian.h>

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
#endif /* BITCOIN_CORPUS_H */
