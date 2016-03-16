#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <MEN/men_z149_dpram.h>
 
#if 0
static int set_int_mode(int fd, int mode)
{
	int int_mode = mode;

	if (ioctl(fd, DPRAM_IOC_SET_INT_MODE_FROM_NIOS, &int_mode) == -1) {
		perror("ioctl set interrupt mode from NIOS");
		return -1;
	}
	if (ioctl(fd, DPRAM_IOC_GET_INT_MODE_FROM_NIOS, &int_mode) == -1) {
		perror("ioctl get interrupt mode from NIOS");
		return -1;
	}

	return 0;
}
#endif

static int get_int_from_nios(int fd, int *int_status)
{
	if (ioctl(fd, DPRAM_IOC_GET_INT_FROM_NIOS, int_status) == -1) {
		perror("ioctl get interrupt from NIOS");
		return -1;
	}

	return 0;
}

static int reset_int_from_nios(int fd)
{
	if (ioctl(fd, DPRAM_IOC_RESET_INT_FROM_NIOS) == -1) {
		perror("ioctl reset interrupt from NIOS");
		return -1;
	}

	return 0;
}

static int set_int_to_nios(int fd)
{
	if (ioctl(fd, DPRAM_IOC_SET_INT_TO_NIOS) == -1) {
		perror("ioctl set interrupt to NIOS");
		return -1;
	}

	return 0;
}

static int get_dpram_info(int fd, struct dpram_info *info)
{
	if (ioctl(fd, DPRAM_IOC_GET_INFO, info) == -1) {
		perror("ioctl get info failed");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	char *dpram = NULL;
	struct dpram_info info;
	char buf[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	int int_status = 0;
	int i;
		 

	/* open file descriptor */
	fd = open("/dev/dpram", O_RDWR);
	if(fd < 0) {
		perror("Open call failed");
		return -1;
	}

	/* get dpram physical address and size */
	if (get_dpram_info(fd, &info) == -1)
		return -1;
	printf("DPRAM: address = 0x%08x, size = 0x%x\n", (unsigned int)info.address, info.size);

	/* map dpram memory to user space */
	dpram = mmap(NULL, info.size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, info.address);
	if (dpram == MAP_FAILED) {
		perror("mmap operation failed");
		return -1;
	}

	/* write to dpram memory at offset 0x100 */
	memcpy(dpram + 0x100, buf, sizeof(buf));

	/* read from dpram memory at offset 0x100 */
	memcpy(buf, dpram + 0x100, sizeof(buf));
	printf("DPRAM data at offset 0x100 =");
	for (i = 0; i < 16; i++)
		printf(" 0x%02x", buf[i]);
	printf("\n");

	/* write to dpram memory at offset 0x100 */
	memset(buf, 0, sizeof(buf));
	memcpy(dpram + 0x100, buf, sizeof(buf));

	/* read from dpram memory at offset 0x100 */
	memcpy(buf, dpram + 0x100, sizeof(buf));
	printf("DPRAM data at offset 0x100 =");
	for (i = 0; i < 16; i++)
		printf(" 0x%02x", buf[i]);
	printf("\n");

	/* send interrupt to NIOS */
	if(!set_int_to_nios(fd))
		printf("Interrupt send to NIOS\n");

	/* poll interrupt from NIOS */
	if (!get_int_from_nios(fd, &int_status)) {
		if (int_status == DPRAM_INT_FROM_NIOS_PENDING)
			printf("Interrupt from NIOS is pending\n");
		else if (int_status == DPRAM_INT_FROM_NIOS_NONE)
			printf("No interrupt from NIOS\n");
	}

	/* reset interrupt from NIOS */
	if(!reset_int_from_nios(fd))
		printf("Interrupt from NIOS reseted\n");

	/* close file descriptor */
	close(fd);
	
	return 0;
}
