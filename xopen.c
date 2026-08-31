/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <regex.h>
#include <unistd.h>
#if MAGIC == 1
#include <stdbool.h>
#include <magic.h>
#endif
#include <spawn.h>
#include <fcntl.h>
#include <sys/wait.h>

/* Access the global system environment variables */
extern char **environ;

typedef struct {
	const char *regex;
	const char **action;
} Pair;

#include "config.h"

static char cmd[BUFSIZ], *cmdv[BUFSIZ/16];
static regmatch_t match[9];

/* precompiled regex cache */
static regex_t *regex_cache = NULL;

/*
 * Wrapper function to launch a process completely detached in the background.
 * Redirects stdin, stdout, and stderr to /dev/null so it doesn't pollute the terminal.
 */
static int
spawn(const char *file, char *const argv[]) {
	pid_t pid;
	posix_spawn_file_actions_t actions;

	/* Initialize the file actions structure */
	if (posix_spawn_file_actions_init(&actions) != 0) {
		perror("posix_spawn_file_actions_init failed");
		return EXIT_FAILURE;
	}

	/* Cleanly redirect child's standard streams to /dev/null so it is detached */
	// 0 = stdin, 1 = stdout, 2 = stderr
	posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
	posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
	posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

	/* Spawn the child process asynchronously with the file modifications */
	int spawn_status = posix_spawnp(&pid, file, &actions, NULL, argv, environ);

	/* Always destroy file actions to prevent memory leaks */
	posix_spawn_file_actions_destroy(&actions);

	if (spawn_status != 0) {
		perror("posix_spawnp failed");
		return EXIT_FAILURE;
	}

	/* Return success immediately to let the parent process die */
	return EXIT_SUCCESS;
}

static void
init_regexes(void) {
	int npairs = sizeof(pairs)/sizeof(*pairs);
	regex_cache = malloc(npairs * sizeof(regex_t));
	if (!regex_cache) {
		perror("malloc failed");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < npairs; i++) {
		if (regcomp(&regex_cache[i], pairs[i].regex, REG_EXTENDED)) {
			errno = EINVAL;
			perror(pairs[i].regex);
			exit(EXIT_FAILURE);
		}
	}
}

static void
cleanup_regexes(void) {
	int npairs = sizeof(pairs)/sizeof(*pairs);
	for (int i = 0; i < npairs; i++) {
		regfree(&regex_cache[i]);
	}
	free(regex_cache);
}

static int
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

	return spawn(*cmdv, cmdv);
}

#if MAGIC == 1
static inline bool
is_prefix(const char *restrict str, const char *restrict prefix, size_t len)
{
        return !strncmp(str, prefix, len);
}

static bool
is_suffix(const char *restrict str, const char *restrict suffix)
{
        if (!str || !suffix)
                return false;

        size_t lenstr = strlen(str);
        size_t lensuffix = strlen(suffix);

        if (lensuffix > lenstr)
                return false;

        return (strcmp(str + (lenstr - lensuffix), suffix) == 0);
}
#endif

int
main(int argc, char *argv[]){
	/* we only take one argument */
	if (argc != 2)
		return EXIT_FAILURE;

	init_regexes();

	/* check regex and launch action if it matches argv[1] */
	for (int i = 0; i < (sizeof(pairs) / sizeof(*pairs)); ++i) {
		if (regexec(&regex_cache[i], argv[1], 9, match, 0) == 0) {
			cleanup_regexes();
			return reexec(argv[1], pairs[i].action);
		}
	}

	cleanup_regexes();

#if MAGIC == 1
	/* initialize magic cookie */
	magic_t magic = magic_open(MAGIC_MIME_TYPE);
	if (magic) {
		/* load the default magic database */
		if (magic_load(magic, NULL) == 0) {
			/* get the MIME type of the file */
			const char *mime = magic_file(magic, argv[1]);
			if (mime) {
				//printf("%s\n", mime);

				if (is_prefix(mime, "image/", 6)) {
					magic_close(magic);
					return reexec(argv[1], pairs[IMAGE_INDEX].action);
				}

				if (is_prefix(mime, "audio/", 6)) {
					magic_close(magic);

					/* mocp cnanot play files without extension */
					char * const args[] = {AUDIO_PLAYER, argv[1], NULL};
					return spawn(AUDIO_PLAYER, args);
				}

				if (is_prefix(mime, "video/", 6)) {
					magic_close(magic);
					return reexec(argv[1], pairs[VIDEO_INDEX].action);
				}

				if (is_suffix(mime, "/pdf")) {
					magic_close(magic);
					return reexec(argv[1], pairs[PDF_INDEX].action);
				}
			}
		}
		magic_close(magic);
	}
#endif

	char * const args[] = {FALLBACK_CMD, argv[1], NULL};
	return spawn(FALLBACK_CMD, args);
}
