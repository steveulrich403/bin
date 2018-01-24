#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *append(char *buf, char *suffix, int suffix_len)
{
        int prefix_len;

        prefix_len = strlen(buf);
        buf = realloc(buf, prefix_len + suffix_len + 1);
        memcpy(buf + prefix_len, suffix, suffix_len);
        buf[prefix_len + suffix_len] = '\0';
        return buf;
        
}

int line_len(char *buf)
{
        char *newline;

        if ((newline = index(buf, '\n')) == NULL)
                return 0;
        return newline - buf;
}

int line_present_in_buffer(char *line, int linelen, char *buf, int max_matches)
{
        char *b = buf;
        char p = line[linelen];
        int num_matches = 0;

        /* null terminate input */
        line[linelen] = '\0';

        /* count # of matching lines */

        while ((b = strstr(b, line)) != NULL) {
                num_matches++;
#if 0
                printf ("%lu\t%s\n\t%-.*s\n", b - buf, line, linelen, b);
#endif
                b += linelen;
        }

        if (num_matches > 1)
                printf ("multiple matches [%d] for:\n '%s'\n", num_matches, line);

        /* restore input */
        line[linelen] = p;

        return (num_matches > 0) && (num_matches <= max_matches);
}

char *read_whole_file(FILE *fp)
{
        char *buf;
        long size;

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        buf = malloc(size + 1);
        fseek(fp, 0, SEEK_SET);
        fread(buf, 1, size, fp);
        return buf;
}

int complexity_score(char *line)
{
        int score = 0;
        int len, i;
        int set[256];

        /* we deal with non-null-terminated lines */

        len = line_len(line);

        /* start off with no copies of each character */

        memset(set, 0, sizeof(set));

        /* iterate, determining how many of each character are present */

        for (i = 0; i < len; i++) {
		set[(unsigned char) line[i]]++;
        }

        for (i = 0; i < sizeof(set) / sizeof(*set); i++) {
                if (set[i] != 0)
                        score++;
        }

        return score;
}

/*
  empirically determine if the patch contained in patch_buf is
  contained in the base directory.  The concept is to look for
  "complex" additions or subtractions that come from the patch. A
  "complex" line is a line containing nontrivial amounts of text,
  with a reasonable number of unique characters.

  A running score is maintained for each test.  As a trial litmus
  test, we decide a patch is present if it has more "present"
  lines than "absent" lines.  This will be revised as we gain
  more experience with real-world patches.
*/

int is_patch_present(char *base_dir, char *patch_buf, char **matches)
{
        char *patch_pos = patch_buf;
	char *full_name;
	int name_len;
	int complexity;
	char *name_pos;
	FILE *test_file_fp;
	char *test_file;
	int present_score = 0;
        int present_possible = 0;

        *matches = strdup("");

	/* iterate over each file's "diff" contents in the patch */

        while (patch_pos &&
               (patch_pos = strstr(patch_pos, "diff --git"))) {
                printf ("Test patch '%-.*s'\n", line_len(patch_pos), patch_pos);

		/* construct filename */

		full_name = malloc(1000);
		strcpy(full_name, base_dir);
		if (full_name[strlen(full_name) - 1] != '/')
                        strcat(full_name, "/");

                name_pos = patch_pos + sizeof("diff --git a"),
                        name_len = index(name_pos, ' ') - name_pos;

                strncat(full_name, name_pos, name_len);

		/* read in the file */

                test_file_fp = fopen(full_name, "r");
		if (test_file_fp == NULL) {
                        printf ("can't open %s for reading\n", full_name);
			return 0;
		}
		test_file = read_whole_file(test_file_fp);
		fclose(test_file_fp);

		/* within each diff, skip the header by advancing past '@@' */
		patch_pos = strstr(patch_pos, "\n@@");
		patch_pos = index(patch_pos + 1, '\n') + 1;

		while (1) {

			/* if find another 'diff --git', it's a new section */

			if (strncmp(patch_pos, "diff --git", sizeof("diff --git") - 1) == 0) {
                                 break;
			}

			complexity = complexity_score(patch_pos);

			if (*patch_pos == '+') {
                                present_possible++;
                                if ((complexity > 15) &&
                                    line_present_in_buffer(patch_pos + 1,
                                                           line_len(patch_pos + 1),
                                                           test_file, 1)) {
                                        present_score++;
                                        *matches = append(*matches, "\t\t", 2);
                                        *matches = append(*matches, patch_pos + 1, line_len(patch_pos + 1));
                                        *matches = append(*matches, "\n", 1);
                                }
			}
			else if (*patch_pos == '-') {
                                /* for now, we don't test deleted lines */
			}

                        /* advance to the next line */

			patch_pos = strchr(patch_pos, '\n');
			if (patch_pos == NULL)
                                break;
			patch_pos++;
		}

		/* done with this subfile */
		free(test_file);

		printf ("\t%d/%d: present\n%s", present_score, present_possible, *matches);
        }

        /* cleanup */
        free(*matches);

        /* for simplicity, start with true if present > 0 */
	return present_score > 0;
}

int main(int argc, char *argv[])
{
        char *git_tree;
        char *patch_file;
        char *matches;

        FILE *patch_fp;
        char *patch_buf;
        int result;

        /* sanity checking */

        if (argc != 3) {
                printf ("usage: %s <git-tree> <patchfile>\n", argv[0]);
                exit(1);
        }

        git_tree = argv[1];
        patch_file = argv[2];

        /* open patch file */

        patch_fp = fopen(patch_file, "r");
        if (patch_fp == NULL) {
                printf ("%s: cannot open %s for reading\n", argv[0], argv[2]);
                exit(1);
        }

        /* read entire patch file into memory */

        patch_buf = read_whole_file(patch_fp);

        /* determine if patch is present in the source tree.  If it isn't,
         * then enough changes have occured in the source tree to obscure
         * the presence of the patch.  (We know this because the patch
         * was generated using the source tree).
         */

        result = is_patch_present(git_tree, patch_buf, &matches);

        printf ("result is %d for patch %s\n", result, argv[2]);
        return result == 0;
}
