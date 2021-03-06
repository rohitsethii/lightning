#include "lightningd/io_loop_with_timers.h"

#include <ccan/io/io.h>
#include <ccan/timer/timer.h>
#include <common/timeout.h>
#include <lightningd/lightningd.h>
#include <wallet/db.h>
#include <wallet/wallet.h>

void *io_loop_with_timers(struct lightningd *ld)
{
	void *retval = NULL;
	struct timer *expired;

	while (!retval) {
		/* ~ccan/io's io_loop() continuously calls
		 * io_poll_lightningd() for all file descriptors registered
		 * with it, then calls their callbacks or closes them if they
		 * fail, as appropriate.
		 *
		 * It will only exit if there's an expired timer, *or* someone
		 * calls io_break, or if there are no more file descriptors
		 * (which never happens in our code). */
		retval = io_loop(ld->timers, &expired);

		/*~ Notice that timers are called here in the event loop like
		 * anything else, so there are no weird concurrency issues. */
		if (expired) {
			db_begin_transaction(ld->wallet->db);
			timer_expired(ld, expired);
			db_commit_transaction(ld->wallet->db);
		}
	}

	return retval;
}
