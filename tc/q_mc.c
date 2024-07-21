/*
 * Multi Queue Scheduler
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"
#include "tc_util.h"

static void explain(void)
{
	fprintf(stderr,
		"Usage: ... mc"
		"		[ maxrate RATE  ] [ sync TIME ]\n"
		);
}

static int mc_parse_opt(struct qdisc_util *qu, int argc, char **argv,
			struct nlmsghdr *n, const char *dev)
{
	unsigned int maxrate = 0;
	unsigned int sync_time = 0;
	bool set_maxrate = false;
	bool set_synctime = false;
	struct rtattr *tail;

	while (argc > 0) {
		if (strcmp(*argv, "maxrate") == 0) {
			NEXT_ARG();
			if (strchr(*argv, '%')) {
				if (get_percent_rate(&maxrate, *argv, dev)) {
					fprintf(stderr, "Illegal \"maxrate\"\n");
					return -1;
				}
			} else if (get_rate(&maxrate, *argv)) {
				fprintf(stderr, "Illegal \"maxrate\"\n");
				return -1;
			}
			set_maxrate = true;
		} else if (strcmp(*argv, "sync") == 0) {
			NEXT_ARG();
			if (get_time(&sync_time, *argv)) {
				fprintf(stderr, "Illegal sync time\n");
				return -1;
			}
			set_synctime = true;
		}
		argc--; argv++;
	}

	fprintf(stderr, "maxrate = %u\n", maxrate);
	fprintf(stderr, "sync time: %u\n", sync_time);
	tail = addattr_nest(n, 1024, TCA_OPTIONS);
	if (set_maxrate)
		addattr_l(n, 1024, TCA_MC_MAX_RATE,&maxrate, sizeof(maxrate));
	if (set_synctime)
		addattr_l(n, 1024, TCA_MC_SYNC_TIME, &sync_time, sizeof(sync_time));
	addattr_nest_end(n, tail);
	return 0;
}

static int mc_print_opt(struct qdisc_util *qu, FILE *f, struct rtattr *opt)
{
	struct rtattr *tb[TCA_MC_MAX + 1];
	unsigned int rate;
	__u64 packets_sent;

	if (opt == NULL)
		return 0;

	parse_rtattr_nested(tb, TCA_MC_MAX, opt);

	if (tb[TCA_MC_MAX_RATE] &&
	    RTA_PAYLOAD(tb[TCA_MC_MAX_RATE]) >= sizeof(__u64)) {
		rate = rta_getattr_u32(tb[TCA_MC_MAX_RATE]);

		if (rate != ~0U)
			tc_print_rate(PRINT_ANY,
				      "maxrate", "maxrate %s ", rate);
	}
	if (tb[TCA_MC_PACKETS_SENT] &&
	    RTA_PAYLOAD(tb[TCA_MC_PACKETS_SENT]) >= sizeof(__u64)) {
		packets_sent = rta_getattr_u64(tb[TCA_MC_PACKETS_SENT]);

		print_u64(PRINT_ANY,
				  "packets sent", "packets sent: %llu ", packets_sent);
	}
	return 0;
}

static int mc_print_xstats(struct qdisc_util *qu, FILE *f,
			   struct rtattr *xstats)
{
	struct tc_fq_qd_stats *st, _st;


	if (xstats == NULL)
		return 0;

	memset(&_st, 0, sizeof(_st));
	memcpy(&_st, RTA_DATA(xstats), min(RTA_PAYLOAD(xstats), sizeof(*st)));

	st = &_st;

	printf("This was executed");

	return 0;
}

struct qdisc_util mc_qdisc_util = {
	.id		= "mc",
	.parse_qopt	= mc_parse_opt,
	.print_qopt	= mc_print_opt,
	.print_xstats	= mc_print_xstats,
};
