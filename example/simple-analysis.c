/* Simple demo to show how to use the corpora ("corpuses"). */
#include <ccan/tal/tal.h>
#include <ccan/tal/grab_file/grab_file.h>
#include <ccan/err/err.h>
#include <assert.h>

#define MIN_BLOCK 352305

#include "../bitcoin-corpus.h"

struct block_stats {
	size_t blocknum;
	size_t known, unknown, mempool;
};

static void reset_stats(struct block_stats *stats)
{
	stats->known = stats->unknown = stats->mempool = 0;
}

static void dump_stats(struct block_stats *stats)
{
	if (stats->blocknum < MIN_BLOCK)
		return;
	printf("%zu: %zu known, %zu unknown, %zu in mempool\n",
	       stats->blocknum, stats->known, stats->unknown, stats->mempool);
}

int main(int argc, char *argv[])
{
	size_t num, i;
	struct block_stats stats;
	struct corpus_entry *e = grab_file(NULL, argv[1]);

	if (tal_count(e) % sizeof(*e) != 1)
		errx(1, "Unexpected length %zu of corpus", tal_count(e));
	num = tal_count(e) / sizeof(*e);

	stats.blocknum = 0;
	reset_stats(&stats);
	for (i = 0; i < num; i++) {
		switch (corpus_entry_type(e+i)) {
		case COINBASE:
			/* Dump previous stats, if any */
			dump_stats(&stats);
			reset_stats(&stats);
			stats.blocknum = corpus_blocknum(e+i);
			break;
		case INCOMING_TX:
			break;
		case UNKNOWN:
			stats.unknown++;
			break;
		case KNOWN:
			stats.known++;
			break;
		case MEMPOOL_ONLY:
			stats.mempool++;
			break;
		default:
			errx(1, "Unknown type %u\n", corpus_entry_type(e+i));
		}
	}
	dump_stats(&stats);
	return 0;
}
