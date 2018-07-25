/*
 *  libfoxenflac -- FLAC decoder
 *  Copyright (C) 2018  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <foxen/flac.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <FLAC FILE>\n", argv[0]);
		return 1;
	}
	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		fprintf(stderr, "Error opening file \"%s\"", argv[1]);
		return 1;
	}

	uint8_t buf[128];
	int32_t out_buf[512];
	uint32_t buf_wr_cur = 0;
/*	fx_flac_t *flac = FX_FLAC_ALLOC_DEFAULT();*/
	fx_flac_t *flac = FX_FLAC_ALLOC_SUBSET_FORMAT_DAT();
	bool done = false;
	while (!done) {
		/* Read data from the input file */
		size_t to_read =  sizeof(buf) - buf_wr_cur;
		if (to_read > 0) {
			size_t n_read = fread(buf + buf_wr_cur, 1, to_read, f);
			if (n_read == 0) {
				fprintf(stderr, "%s: Reached end of file.\n", argv[1]);
				done = true;
				break;
			}

			/* Advance the write cursor */
			buf_wr_cur += n_read;
		}

		/* Read from the buffer */
		uint32_t buf_len = buf_wr_cur;
		uint32_t out_buf_len = 512;
		switch (fx_flac_process(flac, buf, &buf_len, out_buf, &out_buf_len)) {
			case FLAC_END_OF_METADATA:
				fprintf(stderr, "%s: Min/Max Block size %ld/%ld\n", argv[1],
				        fx_flac_get_streaminfo(flac, FLAC_KEY_MIN_BLOCK_SIZE),
				        fx_flac_get_streaminfo(flac, FLAC_KEY_MAX_BLOCK_SIZE));
				break;
			case FLAC_ERR:
				fprintf(stderr, "FLAC decoder in error state!\n");
				done = true;
				break;
			default:
				break;
		}

		/* Write decoded samples to stdout */
		/*fwrite(out_buf, 4, out_buf_len, stdout);*/
		for (uint32_t i = 0U; i < out_buf_len; i++) {
			int16_t out = out_buf[i] >> 16U;
			fwrite(&out, 2, 1, stdout);
		}

		/* Copy unread bytes to the beginning of the buffer and adjust the write
		   cursor. */
		for (uint32_t i = 0; i < buf_wr_cur - buf_len; i++) {
			buf[i] = buf[i + buf_len];
		}
		buf_wr_cur = buf_wr_cur - buf_len;
	}
	free(flac);

	fclose(f);
}