#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <librsync.h>

int main(int argc, char **argv) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s oldfile fromfile tofile\n", argv[0]);
		return 1;
	}

	FILE *oldfile, *sigfile, *deltafile, *fromfile, *tofile;
	rs_signature_t *sumset;
	rs_result r;

	char *sigfile_name = strdup(tmpnam(NULL));
	char *deltafile_name = strdup(tmpnam(NULL));

	mkfifo(sigfile_name, S_IWUSR | S_IRUSR);
	mkfifo(deltafile_name, S_IWUSR | S_IRUSR);

	if (fork() == 0) {	// receiver
		oldfile = fopen(argv[1], "rb");
		deltafile = fopen(deltafile_name, "rb");
		sigfile = fopen(sigfile_name, "wb");
		tofile = fopen(argv[3], "wb");

		// oldfile -> sigfile
		r = rs_sig_file(oldfile, sigfile, RS_DEFAULT_BLOCK_LEN, 8, RS_BLAKE2_SIG_MAGIC, NULL);

		fclose(sigfile);

		// oldfile, deltafile -> tofile
		r = rs_patch_file(oldfile, deltafile, tofile, NULL);

		fclose(oldfile);
		fclose(deltafile);
		fclose(tofile);
	}

	// sender

	deltafile = fopen(deltafile_name, "wb");
	sigfile = fopen(sigfile_name, "rb");
	fromfile = fopen(argv[2], "rb");

	// sigfile -> sumset
	r = rs_loadsig_file(sigfile, &sumset, NULL);
	rs_build_hash_table(sumset);

	// sumset, fromfile -> deltafile
	r = rs_delta_file(sumset, fromfile, deltafile, NULL);

	fclose(deltafile);
	fclose(sigfile);
	fclose(fromfile);

	return 0;
}
