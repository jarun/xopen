/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include <unistd.h>

typedef struct {
	const char *regex;
	const char **action;
} Pair;

#include "config.h"

int i;
char cmd[BUFSIZ], *cmdv[BUFSIZ/16];
regmatch_t match[9];

/* precompiled regex cache */
regex_t *regex_cache = NULL;

void
init_regexes(void) {
	int npairs = sizeof(pairs)/sizeof(*pairs);
	regex_cache = malloc(npairs * sizeof(regex_t));
	if (!regex_cache) {
		fprintf(stderr, "memory allocation failed\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < npairs; i++) {
		if (regcomp(&regex_cache[i], pairs[i].regex, REG_EXTENDED)) {
			fprintf(stderr, "invalid regex: %s\n", pairs[i].regex);
			exit(EXIT_FAILURE);
		}
	}
}

void
cleanup_regexes(void) {
	int npairs = sizeof(pairs)/sizeof(*pairs);
	for (int i = 0; i < npairs; i++) {
		regfree(&regex_cache[i]);
	}
	free(regex_cache);
}

int
reexec(char *uri, const char **args) {
	const char *arg;
	char *p = cmd;
	size_t remaining = BUFSIZ;
	int cmdv_idx = 0;

	while (*args && cmdv_idx < BUFSIZ/16 - 1) {
		arg = *args;
		cmdv[cmdv_idx++] = p;

		while (*arg && remaining > 1) {
			if (*arg == '%') {
				unsigned char nc = *(arg + 1);
				/* check if N in %N is between 0 and 9 */
				int group = nc - '0';
				int len;
				if (group >= 0 && group <= 9 && match[group].rm_so >= 0) {
					len = match[group].rm_eo - match[group].rm_so;
					if (len > remaining - 1) len = remaining - 1;
					snprintf(p, len + 1, "%.*s", len, uri + match[group].rm_so);
				} else /* if (nc == 's') */ {
					len = snprintf(p, remaining, "%s", uri);
					if (len < 0) len = 0;
				}
				arg += 2;
				p += len;
				remaining -= len;
			} else {
				*p++ = *arg++;
				remaining--;
			}
		}

		*p++ = '\0';
		remaining--;
		args++;
	}

	cmdv[cmdv_idx] = NULL;

	return execvp(*cmdv, cmdv);
}


int
main(int argc, char *argv[]){
	/* we only take one argument */
	if (argc != 2)
		return EXIT_FAILURE;

	init_regexes();

	/* check regex and launch action if it matches argv[1] */
	for (i=0; i < sizeof(pairs)/sizeof(*pairs); ++i) {
		if (regexec(&regex_cache[i], argv[1], 9, match, 0) == 0) {
			cleanup_regexes();
			return reexec(argv[1], pairs[i].action);
		}
	}

	cleanup_regexes();

	/* alternatively, fall back to chndlr_fallback_cmd */
	int len = strlen(chndlr_fallback_cmd) + strlen(argv[1]) + 1;
	if (len > BUFSIZ) {
		fprintf(stderr, "command too long\n");
		return EXIT_FAILURE;
	}
	snprintf(cmd, BUFSIZ, "%s%s", chndlr_fallback_cmd, argv[1]);
	return system(cmd);
}
