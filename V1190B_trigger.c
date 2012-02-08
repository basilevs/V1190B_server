#include "stdio.h"
#include "unistd.h"
#include "libvmedirect.h"
#include "V1190B.h"



int main(int argc, char * argv[]) {
	int count=1;
	int opt;
	printf("BEAM V1190B software trigger\n");
    while ((opt = getopt(argc, argv, "c:")) != -1) {
         switch (opt) {
         case 'c':
             count = atoi(optarg);
             if (count < 0) count=0;
             break;
         default: /* '?' */
             fprintf(stderr, "Usage: %s [-c count]\n",
                     argv[0]);
             exit(1);
         }
     }

	if (libvme_init() < 0) {
		printf("libvme_init failed\n");
		return 1;
	}
	BusAddr tdcAddr=0xEE000000;
	while (count > 0) {
		//Software trigger
		if (libvme_write_a32_word(tdcAddr+TDC_SOFTRIGGER_REGISTER, 0)!=0) {
			printf("Unable to raise software trigger\n");
		}
		--count;
	}
	libvme_close();
	return 0;
}
