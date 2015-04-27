#include <ccan/rbuf/rbuf.h>
#include <ccan/err/err.h>
#include <ccan/str/str.h>
#include <ccan/read_write_all/read_write_all.h>
#include <ccan/htable/htable_type.h>
#include <ccan/tal/tal.h>
#include "../bitcoin-corpus.h"
#include <stdlib.h>
#include <unistd.h>

static bool char_to_hex(u8 *val, char c)
{
	if (c >= '0' && c <= '9') {
		*val = c - '0';
		return true;
	}
 	if (c >= 'a' && c <= 'f') {
		*val = c - 'a' + 10;
		return true;
	}
 	if (c >= 'A' && c <= 'F') {
		*val = c - 'A' + 10;
		return true;
	}
	return false;
}

static bool from_hex(const char *str, size_t slen, void *buf, size_t bufsize)
{
	u8 v1, v2;
	u8 *p = buf;

	while (slen > 1) {
		if (!char_to_hex(&v1, str[0]) || !char_to_hex(&v2, str[1]))
			return false;
		if (!bufsize)
			return false;
		*(p++) = (v1 << 4) | v2;
		str += 2;
		slen -= 2;
		bufsize--;
	}
	return slen == 0 && bufsize == 0;
}

static bool get_long(const char *word, size_t line, unsigned long *l)
{
	char *endp;

	if (!word)
		return false;

	*l = strtol(word, &endp, 10);
	if (*l == ULONG_MAX)
		err(1, "Parsing line %zu number '%s'", line, word);
	if (*endp)
		errx(1, "Line %zu invalid number '%s'", line, word);

	return true;
}

static const struct corpus_txid *get_txid(const struct corpus_txid *txid)
{
	return txid;
}
static size_t txid_hash(const struct corpus_txid *txid)
{
	size_t n;
	memcpy(&n, txid, sizeof(n));
	return n;
}
static bool txid_eq(const struct corpus_txid *a, const struct corpus_txid *b)
{
	return memcmp(a, b, sizeof(*a)) == 0;
}

HTABLE_DEFINE_TYPE(struct corpus_txid, get_txid, txid_hash, txid_eq, txhash);
static struct txhash coinbases;

static void load_coinbases(const char *coinbase_file)
{
	struct rbuf rbuf;
	const char *word;
	size_t line = 0;

	if (!rbuf_open(&rbuf, coinbase_file, NULL, 0))
		err(1, "Opening %s", coinbase_file);

	txhash_init(&coinbases);

	/* txid */
	while ((word = rbuf_read_str(&rbuf, '\n', realloc)) != NULL) {
		struct corpus_txid *txid = tal(NULL, struct corpus_txid);

		line++;
		if (!from_hex(word, strlen(word), txid, sizeof(*txid)))
			errx(1, "Bad coinbase txid in line %zu", line);

		txhash_add(&coinbases, txid);
	}
}	

static bool is_coinbase(const struct corpus_txid *txid)
{
	return txhash_get(&coinbases, txid) != NULL;
}

static void dump_prev_block(struct corpus_entry **cb,
			    struct corpus_entry *txs,
			    size_t *num_txs,
			    size_t line)
{
	/* Nothign to dump? */
	if (*num_txs == 0 && !*cb)
		return;

	if (!*cb)
		errx(1, "No coinbase line %zu", line);

	/* Always write coinbase first, then rest of block. */
	if (!write_all(STDOUT_FILENO, *cb, sizeof(**cb))
	    || !write_all(STDOUT_FILENO, txs, sizeof(*txs) * *num_txs))
		err(1, "Writing out entries");
	*num_txs = 0;
	*cb = tal_free(*cb);
}

int main(int argc, char *argv[])
{
	struct rbuf rbuf;
	size_t line = 0, num_txs = 0;
	unsigned long l, prev_blocknum = 0;
	struct corpus_entry *txs = tal_arr(NULL, struct corpus_entry, 128), *cb;
	enum corpus_entry_type prev_type = INCOMING_TX;

	if (argc == 2)
		rbuf_init(&rbuf, STDIN_FILENO, NULL, 0);
	else if (argc == 3) {
		if (!rbuf_open(&rbuf, argv[2], NULL, 0))
			err(1, "Opening %s", argv[2]);
	} else
		errx(1, "Usage: encode-dump <coinbases> [<filename>]");

	load_coinbases(argv[1]);

	cb = NULL;

	/* <timestamp>[:<blocknum>]:<op>:<txid> */
	while (get_long(rbuf_read_str(&rbuf, ':', realloc), ++line, &l)) {
		char *word;
		enum corpus_entry_type t;
		struct corpus_entry e;

 		e.timestamp = le32_to_cpu(l);

		word = rbuf_read_str(&rbuf, ':', realloc);
		if (!word)
			errx(1, "Short line %zu", line);

		if (streq(word, "add")) {
			t = INCOMING_TX;
			l = 0;
		} else {
			/* Block number first */
			if (!get_long(word, line, &l))
				errx(1, "Short line %zu", line);

			if (l >= (1 << 24))
				errx(1, "Line %zu block '%s' too large",
				     line, word);

			word = rbuf_read_str(&rbuf, ':', realloc);
			if (!word)
				errx(1, "Short line %zu", line);

			if (streq(word, "mutual"))
				t = KNOWN;
			else if (streq(word, "new"))
				t = UNKNOWN;
			else if (streq(word, "mempool-only"))
				t = MEMPOOL_ONLY;
			else
				errx(1, "Unknown type '%s' on line %zu",
				     word, line);
		}
		word = rbuf_read_str(&rbuf, '\n', realloc);
		if (!word)
			errx(1, "No txid in line %zu", line);
		if (!from_hex(word, strlen(word), &e.txid, sizeof(e.txid)))
			errx(1, "Bad txid in line %zu", line);

		/* Figure out if it's a coinbase transaction */
		if (t == UNKNOWN && is_coinbase(&e.txid))
			t = COINBASE;

		e.type_and_blocknum = cpu_to_le32(t | (l << 8));

		/* Input order is always:
		 *   INCOMING* (KNOWN|UNKNOWN)+ MEMPOOL_ONLY*
		 *
		 * So in theory two blocks with no mempool-only and no incoming
		 * txs between them could merge together.  My dump used block
		 * height, not actual hash :(
		 *
		 * This doesn't happen in our dumps though, so we
		 * detect the end of a block as follows:
		 * 1) Previous tx was mempool_only, this isn't, OR
		 * 2) Previous tx's blocknum was != this tx, OR
		 * 3) This tx is INCOMING.
		 */
		if ((prev_type == MEMPOOL_ONLY && t != MEMPOOL_ONLY)
		    || prev_blocknum != l
		    || prev_type == INCOMING_TX) {
			dump_prev_block(&cb, txs, &num_txs, line);
		}
		
		if (t == COINBASE) {
			cb = tal(txs, struct corpus_entry);
			*cb = e;
		} else if (t == INCOMING_TX) {
			if (!write_all(STDOUT_FILENO, &e, sizeof(e)))
				err(1, "Writing out entry");
		} else {
			/* Save up entries, so coinbase always first! */
			if (num_txs >= tal_count(txs))
				tal_resize(&txs, num_txs * 2);
			txs[num_txs++] = e;
		}
		prev_type = t;
		prev_blocknum = l;
	}

	dump_prev_block(&cb, txs, &num_txs, line);

	return 0;
}
