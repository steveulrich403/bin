#include <stdio.h>
#include <ctype.h>

main()
{
	int c;
	int shacount = 0;
  	char sha[1024];

	while ((c = getchar()) != EOF) {
		if (isxdigit(c)) {
			if (shacount >= sizeof(sha)) {
				printf ("buffer overflow\n");
			    return 1;
			}
			sha[shacount++] = c;
		}
		else {
			sha[shacount++] = 0;
			if (shacount >= 8)
			    printf ("%s\n", sha);
			shacount = 0;
		}
	}
}
