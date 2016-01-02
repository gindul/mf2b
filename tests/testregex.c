#define _POSIX_C_SOURCE 201312L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

int main(int argc, char **argv)
{
	regmatch_t match[128];
	regex_t preg;
	int i;
	char *s;

	if (argc != 3) {
		printf("usage: %s regex string\n", argv[0]);
		return 1;
	}

	if (regcomp(&preg, argv[1], 0)) {
		perror("regcomp()");
		return 1;
	}

	if (regexec(&preg, argv[2], 128, match, 0)) {
		perror("regexec()");
		return 1;
	}
	for (i = 0; i < 128; i++) {
		if (match[i].rm_so == -1)
			continue;
		s = strndup(argv[2] + match[i].rm_so,
				match[i].rm_eo - match[i].rm_so);
		printf("match[%.000d] = %s\n", i, s);
		free(s);
	}
	return 0;
}
