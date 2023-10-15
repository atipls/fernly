#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <time.h>

#define BAUDRATE B115200
#define STAGE_2_WRITE_ALL_AT_ONCE 0 /* Write stage 2 in one write() */
#define STAGE_2_WRITE_SIZE 1
#define STAGE_3_WRITE_ALL_AT_ONCE 1 /* Write stage 3 in one write() */
#define STAGE_3_WRITE_SIZE 1
#define FERNLY_USB_LOADER_ADDR 0x70009000

#define ASSERT(x) do { if ((x)) exit(1); } while(0)

static const uint32_t mtk_config_offset = 0x80000000;
static struct termios old_termios;
const char prompt[] = "fernly>";

enum mtk_commands {
	mtk_cmd_old_write16 = 0xa1,
	mtk_cmd_old_read16 = 0xa2,
	mtk_checksum16 = 0xa4,
	mtk_remap_before_jump_to_da = 0xa7,
	mtk_jump_to_da = 0xa8,
	mtk_send_da = 0xad,
	mtk_jump_to_maui = 0xb7,
	mtk_get_version = 0xb8,
	mtk_close_usb_and_reset = 0xb9,
	mtk_cmd_new_read16 = 0xd0,
	mtk_cmd_new_read32 = 0xd1,
	mtk_cmd_new_write16 = 0xd2,
	mtk_cmd_new_write32 = 0xd4,
//	mtk_jump_to_da = 0xd5,
	mtk_jump_to_bl = 0xd6,
	mtk_get_sec_conf = 0xd8,
	mtk_send_cert = 0xe0,
	mtk_get_me = 0xe1,		/* Responds with 22 bytes */
	mtk_send_auth = 0xe2,
	mtk_sla_flow = 0xe3,
	mtk_send_root_cert = 0xe5,
	mtk_do_security = 0xfe,
	mtk_firmware_version = 0xff,
};

/* "general file header", struct gfh_header is actually a lead-in
   for different header types (as specified by type field). */
struct gfh_header {
	uint32_t magic_ver; /* 'MMM', highest byte - version */
	uint16_t size;      /* Total header size, incl. struct gfh_header */
	uint16_t type;      /* 0 - gfh_file_info */
};

struct gfh_file_info {
	struct gfh_header header;
	uint8_t		id[12]; /* "FILE_INFO", zero-padded */
	uint32_t	file_ver;
	uint16_t	file_type;
	uint8_t		flash_dev;
	uint8_t		sig_type;
	uint32_t	load_addr;
	uint32_t	file_len;
	uint32_t	max_size;
	uint32_t	content_offset;
	uint32_t	sig_len;
	uint32_t	jump_offset;
	uint32_t	attr;
};

static const uint8_t mtk_banner[]	   = { 0xa0, 0x0a, 0x50, 0x05 };
static const uint8_t mtk_banner_response[] = { 0x5f, 0xf5, 0xaf, 0xfa };

static void restore_termios(void)
{
	tcsetattr(1, TCSANOW, &old_termios);
}

int print_hex_offset(const void *block, int count, int offset, uint32_t start)
{
	int byte;
	const uint8_t *b = block;
	count += offset;
	b -= offset;
	for ( ; offset < count; offset += 16) {
		printf("%08x", start + offset);

		for (byte = 0; byte < 16; byte++) {
			if (byte == 8)
				printf(" ");
			printf(" ");
			if (offset + byte < count)
				printf("%02x", b[offset + byte] & 0xff);
			else
				printf("  ");
		}

		printf("  |");
		for (byte = 0; byte < 16 && byte + offset < count; byte++)
			printf("%c", isprint(b[offset + byte]) ?
				  b[offset + byte] :
				  '.');
		printf("|\n");
	}
	return 0;
}

int print_hex(const void *block, int count, uint32_t start)
{
	return print_hex_offset(block, count, 0, start);
}

static int fernvale_txrx(int fd, void *b, int size) {
	uint8_t response[size];
	uint8_t *bfr = b;
	int ret;

//	printf("Writing data: "); print_hex(bfr, size, 0);
	ret = write(fd, bfr, size);
	if (ret != size) {
		if (ret == -1)
			perror("Unable to write buffer");
		else
			printf("Wanted to write %d bytes, but read %d\n",
					size, ret);
		return -1;
	}

	ret = read(fd, response, size);
	if (ret != size) {
		if (ret == -1)
			perror("Unable to read response");
		else
			printf("Wanted to read %d bytes, but read %d (%x)\n",
					size, ret, response[0]);
		return -1;
	}
//	printf("Reading data: "); print_hex(response, size, 0);

	if (memcmp(bfr, response, size)) {
		fprintf(stderr, "Error: Response differs from command\n");
		int i;
		printf(" Expected: ");
		for (i = 0; i < size; i++)
			printf(" %02x", bfr[i] & 0xff);
		printf("\n Received: ");
		for (i = 0; i < size; i++)
			printf(" %02x", response[i] & 0xff);
		printf("\n");
		return -1;
	}
	return 0;
}

int fernvale_send_cmd(int fd, uint8_t bfr) {
	return fernvale_txrx(fd, &bfr, sizeof(bfr));
}

int fernvale_send_int16(int fd, uint16_t word) {
	uint8_t bfr[2];
	bfr[0] = word >> 8;
	bfr[1] = word >> 0;
	return fernvale_txrx(fd, bfr, sizeof(bfr));
}

uint16_t fernvale_get_int8(int fd) {
	uint8_t bfr;
	int ret;

	ret = read(fd, &bfr, sizeof(bfr));
	if (ret != sizeof(bfr)) {
		perror("Unable to read 16 bits");
		return -1;
	}
	return bfr;
}

int fernvale_get_int16(int fd) {
	uint16_t bfr, out;
	int ret;

	ret = read(fd, &bfr, sizeof(bfr));
	if (ret != sizeof(bfr)) {
		perror("Unable to read 16 bits");
		return -1;
	}
	out = ((bfr >> 8) & 0x00ff)
	    | ((bfr << 8) & 0xff00);
	return out;
}

uint32_t fernvale_get_int32(int fd) {
	uint32_t bfr, out;
	int ret;

	ret = read(fd, &bfr, sizeof(bfr));
	if (ret != sizeof(bfr)) {
		perror("Unable to read 32 bits");
		return -1;
	}

	out = ((bfr >> 24) & 0x000000ff)
	    | ((bfr >>  8) & 0x0000ff00)
	    | ((bfr <<  8) & 0x00ff0000)
	    | ((bfr << 24) & 0xff000000);
	return out;
}

static int fernvale_send_int32(int fd, uint32_t word) {
	uint8_t bfr[4];
	bfr[0] = word >> 24;
	bfr[1] = word >> 16;
	bfr[2] = word >> 8;
	bfr[3] = word >> 0;
	return fernvale_txrx(fd, bfr, sizeof(bfr));
}

int fernvale_send_int8_no_response(int fd, uint8_t byte) {

	int ret;
	ret = write(fd, &byte, sizeof(byte));
	if (ret != sizeof(byte)) {
		if (ret == -1)
			perror("Unable to write buffer");
		else
			printf("Wanted to write %d bytes, but read %d\n",
					(int) sizeof(byte), ret);
		return -1;
	}
	return 0;
}

int fernvale_send_int16_no_response(int fd, uint32_t word) {
	uint8_t bfr[2];
	int ret;

	bfr[0] = word >> 8;
	bfr[1] = word >> 0;

	ret = write(fd, bfr, sizeof(bfr));
	if (ret != sizeof(bfr)) {
		if (ret == -1)
			perror("Unable to write buffer");
		else
			printf("Wanted to write %d bytes, but read %d\n",
					(int) sizeof(bfr), ret);
		return -1;
	}
	return 0;
}

int fernvale_send_int32_no_response(int fd, uint32_t word) {
	uint8_t bfr[4];
	int ret;

	bfr[0] = word >> 24;
	bfr[1] = word >> 16;
	bfr[2] = word >> 8;
	bfr[3] = word >> 0;

	ret = write(fd, bfr, sizeof(bfr));
	if (ret != sizeof(bfr)) {
		if (ret == -1)
			perror("Unable to write buffer");
		else
			printf("Wanted to write %d bytes, but read %d\n",
					(int) sizeof(bfr), ret);
		return -1;
	}
	return 0;
}

int fernvale_cmd_jump(int fd, uint32_t addr) {
	int ret;

	ret = fernvale_send_cmd(fd, 0xd5);
	if (ret) {
		fprintf(stderr, "Unable to send jump command ");
		return ret;
	}

	ret = fernvale_send_int32(fd, addr);
	if (ret) {
		fprintf(stderr, "Unable to send address ");
		return ret;
	}

	ret = fernvale_get_int16(fd);
	if (ret) {
		fprintf(stderr, "Error while jumping: 0x%04x ", ret);
		return ret;
	}

	return 0;
}

int fernvale_cmd_send_fd(int fd, uint32_t addr, int binfd)
{
	struct stat stats;
	uint16_t checksum, checksum_calc;
	uint16_t response;
	//uint32_t signature_len = 1024; // Number of 32-bit words to send (?)
	uint32_t signature_len = 2; // Size of the signature hash
//	uint32_t unk1 = 0x00000001;
	uint32_t size;
	uint8_t *bfr;

	if (-1 == fstat(binfd, &stats)) {
		perror("Unable to get file stats");
		close(binfd);
		return -1;
	}

	if (addr >= 0x70000000 && addr < 0x70006598) {
		printf("warning: address is probably invalid ");
		fflush(stdout);
	}

	size = stats.st_size;

	bfr = malloc(size);
	if (!bfr) {
		perror("Unable to allocate xmit buffer");
		return -1;
	}

	if (size != read(binfd, bfr, size)) {
		perror("Unable to read file for sending");
		close(binfd);
		free(bfr);
		return -1;
	}

	close(binfd);

	fernvale_send_cmd(fd, 0xd7);
	fernvale_send_int32(fd, addr);
	fernvale_send_int32(fd, size);
	fernvale_send_int32(fd, signature_len);

	/* Not sure what this response is, but it's always 0 */
	response = fernvale_get_int16(fd);
	if (response != 0)
		printf("!! First response is 0x%04x, not 0 !!\n", response);

	bfr[size - 1] ^= 0xff;
	response = write(fd, bfr, size);
	if (response != size) {
		if (response == 0xFFFF)
			perror("Unable to write file");
		else {
			printf("Wanted to write %d bytes of file, got %d ",
				size, response);
			fflush(stdout);
		}
		free(bfr);
		return -1;
	}

	/* Calculate a checksum of the whole file */
	checksum = fernvale_get_int16(fd);
	{
		int i;
		uint16_t *checksum_ptr = (uint16_t *)bfr;
		checksum_calc = 0;
		for (i = 0; i < stats.st_size; i += 2)
			checksum_calc ^= *checksum_ptr++;
	}
	if (checksum == checksum_calc)
		printf("checksum matches 0x%04x ", checksum);
	else
		printf("device checksum 0x%04x, but we calculated 0x%04x ",
			checksum, checksum_calc);

	/* Not sure what this response is, but it's always 0 */
	response = fernvale_get_int16(fd);
	if (response != 0)
		printf("!! Final response is 0x%04x, not 0 !!\n", response);

	free(bfr);
	return 0;
}

int fernvale_send_bootloader(int fd, uint32_t addr, uint32_t stack,
			uint32_t unk, const char *filename) {

	struct stat stats;
	struct gfh_file_info *info;
	uint16_t checksum, checksum_calc;
	uint32_t size;
	int ret;
	int tmp;
	int binfd;

	ret = 0;

	binfd = open(filename, O_RDONLY);
	if (-1 == binfd) {
		perror("Unable to open bootloader for sending");
		return -1;
	}

	if (-1 == fstat(binfd, &stats)) {
		perror("Unable to get file stats");
		close(binfd);
		return -1;
	}

	size = stats.st_size;

	uint8_t bfr[size];

	if (size != read(binfd, bfr, size)) {
		perror("Unable to read file for sending");
		close(binfd);
		return -1;
	}

	close(binfd);

	/* XXX PATCH BOOTLOADER */
	info = (struct gfh_file_info *)bfr;
	printf("Id: ");
	int j;
	for (j = 0; j < 12; j++)
		printf("%c", info->id[j]);
	printf("\n");
	printf("Version: %d\n", info->file_ver);
	printf("Type: %d\n", info->file_type);
	printf("Flash device: %d\n", info->flash_dev);
	printf("File size: %d\n", info->file_len);
	printf("Max size: %d\n", info->max_size);
	printf("Signature type: %d\n", info->sig_type);
	printf("Signature length: %d\n", info->sig_len);
	printf("Load address: 0x%08x\n", info->load_addr);
	printf("Content offset: %d\n", info->content_offset);
	printf("Jump offset: %d\n", info->jump_offset);
	printf("Attributes: %d\n", info->attr);

	printf("Hash:\n");
	print_hex(bfr + size - info->sig_len, info->sig_len, 0);

	fernvale_send_cmd(fd, 0xd9);
	fernvale_send_int32(fd, addr);
	fernvale_send_int32(fd, size);	// Number of bytes
	fernvale_send_int32(fd, stack);
	fernvale_send_int32(fd, unk);
	tmp = fernvale_get_int16(fd);

	if (tmp != 0) {
		printf("Response 0xd9 (1) was not 0, was 0x%04x\n", tmp);
		ret = -1;
	}

	write(fd, bfr, size);
	{
		int i;
		uint16_t *checksum_ptr = (uint16_t *)bfr;
		checksum_calc = 0;
		for (i = 0; i < sizeof(bfr); i += 2)
			checksum_calc ^= *checksum_ptr++;
	}
	checksum = fernvale_get_int16(fd);
	if (checksum == checksum_calc)
		printf("Checksum matches: 0x%04x\n", checksum);
	else
		printf("Checksum differs.  Received: 0x%04x  Calculated: 0x%04x)\n", checksum, checksum_calc);

	tmp = fernvale_get_int16(fd);
	if (tmp != 0) {
		printf("Response 0xd9 (2) was not 0, was 0x%04x\n", tmp);
		ret = -1;
	}

	tmp = fernvale_get_int32(fd);
	if (tmp != 0) {
		printf("Response 0xd9 (4) was not 0, was 0x%08x\n", tmp);
		ret = -1;
	}

	return ret;
}

static int fernvale_print_me(int fd) {
	int ret;
	uint32_t size;
	uint16_t footer;

	ret = fernvale_send_cmd(fd, mtk_get_me);
	if (ret)
		return ret;

	size = fernvale_get_int32(fd);

	uint8_t bfr[size];

	ret = read(fd, bfr, size);
	if (ret != size) {
		perror("Unable to read from ME buffer");
		return -1;
	}

	footer = fernvale_get_int16(fd);
	(void)footer;

	print_hex(bfr, size, 0);

	return 0;
}

static int fernvale_print_sec_conf(int fd) {
	int ret;
	uint32_t size;
	uint16_t footer;

	ret = fernvale_send_cmd(fd, mtk_get_sec_conf);
	if (ret)
		return ret;

	size = fernvale_get_int32(fd);

	uint8_t bfr[size];

	ret = read(fd, bfr, size);
	if (ret != size) {
		perror("Unable to read from Sec Conf buffer");
		return -1;
	}

	footer = fernvale_get_int16(fd);
	(void)footer;

	if (size)
		print_hex(bfr, size, 0);
	else
		printf("None.\n");

	return 0;
}

static int fernvale_send_do_security(int fd) {
	return fernvale_send_cmd(fd, mtk_do_security);
}

static int fernvale_security_version(int fd) {
	int ret;
	uint8_t bfr[1];

	bfr[0] = mtk_firmware_version;
	ret = write(fd, bfr, 1);
	if (ret != 1) {
		perror("Unable to write security character");
		return -1;
	}
	ret = read(fd, bfr, sizeof(bfr));
	if (ret != sizeof(bfr)) {
		if (ret == -1)
			perror("Unable to read response");
		else {
			printf("Wanted to read %d bytes, but got %d", 1, ret);
			fflush(stdout);
		}
		return -1;
	}

	if (ret == 0xff)
		return 0;

	return bfr[0];
}

static int fernvale_hello(int fd) {
	int i;
	int ret;
	uint8_t bfr[1];

	for (i = 0; i < sizeof(mtk_banner); i++) {
		bfr[0] = mtk_banner[i];
		ret = write(fd, bfr, sizeof(bfr));
		if (ret != sizeof(bfr)) {
			perror("Unable to write banner character");
			return -1;
		}

		ret = read(fd, bfr, sizeof(bfr));
		if (ret != sizeof(bfr)) {
			if (ret == -1)
				perror("Unable to read response");
			else {
				printf("Wanted to read %d bytes, but got %d ",
						1, ret);
				fflush(stdout);
			}
			return -1;
		}

		if (bfr[0] != mtk_banner_response[i]) {
			fprintf(stderr,
				"Invalid banner response for "
				"character %d: 0x%02x (wanted 0x%02x) ",
					i, bfr[0], mtk_banner_response[i]);
			return -1;
		}
	}

	return 0;
}

static int fernvale_memory_read(int fd, uint32_t addr, uint32_t count,
				void *bfr) {
	int ret;
	uint8_t *b = bfr;
	int i;

	ret = fernvale_send_cmd(fd, mtk_cmd_old_read16);
	if (ret)
		return ret;

	ret = fernvale_send_int32(fd, addr);
	if (ret)
		return ret;

	/* "Count" is in units of 16-bit words, not 8-bit bytes */
	ret = fernvale_send_int32(fd, count / 2);
	if (ret)
		return ret;

	ret = read(fd, bfr, count);
	if (ret != count) {
		if (ret == -1)
			perror("Unable to read data");
		else
			fprintf(stderr, "Wanted %d bytes, got %d bytes\n",
					count, ret);
		return -1;
	}

	/* Swap bytes around as necessary */
	for (i = 0; i < count; i += 2) {
		uint8_t t = b[i];
		b[i] = b[i + 1];
		b[i + 1] = t;
	}

	return 0;
}

int fernvale_memory_write(int fd, uint32_t addr, uint32_t count,
				 void *bfr) {
	int ret;
	uint8_t *b = bfr;

	if (count & 1)
		count++;

	ret = fernvale_send_cmd(fd, mtk_cmd_old_write16);
	if (ret)
		return ret;

	ret = fernvale_send_int32(fd, addr);
	if (ret)
		return ret;

	/* "Count" is in units of 16-bit words, not 8-bit bytes */
	ret = fernvale_send_int32(fd, count / 2);
	if (ret)
		return ret;

	/* Doubly-swapped bytes */
	while (count > 3) {
		uint16_t data;
		
		data = (b[2] << 8) | (b[3]);

		printf("Writing data: 0x%04x\n", data);
		ret = fernvale_txrx(fd, &data, 2);
		if (ret) {
			perror("Unable to write data");
			return -1;
		}

		data = (b[0] << 8) | (b[1]);

		printf("Writing data: 0x%04x\n", data);
		ret = fernvale_txrx(fd, &data, 2);
		if (ret) {
			perror("Unable to write data");
			return -1;
		}
		b += 4;
		count -= 4;
	}

	return 0;
}

int fernvale_write_reg16(int fd, uint32_t addr, uint16_t val) {
	int ret;

	ret = fernvale_send_cmd(fd, mtk_cmd_old_write16);
	if (ret)
		return ret;

	ret = fernvale_send_int32(fd, addr);
	if (ret)
		return ret;

	/* "Count" is in units of 16-bit words, not 8-bit bytes */
	ret = fernvale_send_int32(fd, 1);
	if (ret)
		return ret;

	return fernvale_send_int16(fd, val);
}

int fernvale_write_reg32(int fd, uint32_t addr, uint32_t val) {
	int ret;

	ret = fernvale_send_cmd(fd, mtk_cmd_old_write16);
	if (ret)
		return ret;

	ret = fernvale_send_int32(fd, addr);
	if (ret)
		return ret;

	/* "Count" is in units of 16-bit words, not 8-bit bytes */
	ret = fernvale_send_int32(fd, 2);
	if (ret)
		return ret;

//	ret = fernvale_send_int32(fd, val);
	fernvale_send_int16(fd, val >> 16);
	fernvale_send_int16(fd, val);
	return 0;
}

uint16_t fernvale_read_reg16(int fd, uint32_t addr) {
	uint16_t val;
	fernvale_memory_read(fd, addr, 2, &val);
	return val;
}

uint32_t fernvale_read_reg32(int fd, uint32_t addr) {
	uint32_t val;
	fernvale_memory_read(fd, addr, 4, &val);
	return val;
}

int fernvale_reset(void) {
	int fd;
	uint8_t b;

	fd = open("/sys/class/gpio/gpio17/value", O_WRONLY);
	b = '0';
	write(fd, &b, 1);
	close(fd);
	
	usleep(250000);

	fd = open("/sys/class/gpio/gpio17/value", O_WRONLY);
	b = '1';
	write(fd, &b, 1);
	close(fd);

	return 0;
}

int write_pattern(int fd) {
	int i;

	for (i = 0; i < 16; i += 2)
		fernvale_write_reg16(fd, 0x70000000 + i, i | ((i + 1) << 8));
	/*
		bfr[i] = i;
	for (i = 0; i < sizeof(bfr); i += 2)
		fernvale_memory_write(fd, 0x70000000 + i, 2, bfr + i);
		*/

//	return fernvale_memory_write(fd, 0x70000000, sizeof(bfr), bfr);
	return 0;
}

int read_pattern(int fd) {
	uint8_t bfr[16];
	uint32_t offset = 0x70006600;

	for (; ; offset += sizeof(bfr)) {
		if (fernvale_memory_read(fd, offset, sizeof(bfr), bfr))
			return -1;
		print_hex(bfr, sizeof(bfr), offset);
	}

	return 0;
}

static uint16_t fernvale_read16(int fd, uint32_t addr) {
	uint16_t ret;
	uint16_t tmp;
	int count = 1;

	fernvale_send_cmd(fd, mtk_cmd_new_read16);
	fernvale_send_int32(fd, addr);
	fernvale_send_int32(fd, count);
	tmp = fernvale_get_int16(fd);
	if (tmp != 0)
		printf("Response read16 (1) was not 0, was 0x%04x\n", tmp);
	ret = fernvale_get_int16(fd);

	tmp = fernvale_get_int16(fd);
	if (tmp != 0)
		printf("Response read16 (3) was not 0, was 0x%04x\n", tmp);

	return ret;
}

static int fernvale_read32(int fd, uint32_t addr) {
	int ret;
	int tmp;
	int count = 1;

	fernvale_send_cmd(fd, mtk_cmd_new_read32);
	fernvale_send_int32(fd, addr);
	fernvale_send_int32(fd, count);

	tmp = fernvale_get_int16(fd);
	if (tmp != 0)
		printf("Response read32 (1) was not 0, was 0x%04x\n", tmp);

	ret = fernvale_get_int32(fd);

	tmp = fernvale_get_int16(fd);
	if (tmp != 0)
		printf("Response read32 (3) was not 0, was 0x%04x\n", tmp);

	return ret;
}

static int fernvale_write16(int fd, uint32_t addr, uint16_t val) {
	int tmp;
	int count = 1;
	int ret = 0;

	fernvale_send_cmd(fd, mtk_cmd_new_write16);
	fernvale_send_int32(fd, addr);
	fernvale_send_int32(fd, count);

	tmp = fernvale_get_int16(fd);
	if (tmp != 1) {
		printf("Response write16 (1) was not 1, was 0x%04x\n", tmp);
		ret = -1;
	}

	fernvale_send_int16(fd, val);

	tmp = fernvale_get_int16(fd);
	if (tmp != 1) {
		printf("Response write16 (2) was not 1, was 0x%04x\n", tmp);
		ret = -1;
	}

	return ret;
}

static int fernvale_write32(int fd, uint32_t addr, uint32_t val) {
	int tmp;
	int ret;
	int type = 1;

	ret = 0;

	fernvale_send_cmd(fd, mtk_cmd_new_write32);
	fernvale_send_int32(fd, addr);
	fernvale_send_int32(fd, type);

	tmp = fernvale_get_int16(fd);
	if (tmp != 1) {
		printf("Response write32 (1) was not 1, was 0x%04x\n", tmp);
		ret = -1;
	}

	fernvale_send_int32(fd, val);

	tmp = fernvale_get_int16(fd);
	if (tmp != 1) {
		printf("Response write32 (2) was not 1, was 0x%04x\n", tmp);
		ret = -1;
	}

	return ret;
}

int fernvale_set_serial(int serfd) {
	int ret;
	struct termios t;

	ret = tcgetattr(serfd, &t);
	if (-1 == ret) {
		perror("Failed to get attributes");
		exit(1);
	}
	cfsetispeed(&t, BAUDRATE);
	cfsetospeed(&t, BAUDRATE);
	cfmakeraw(&t);
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 100;	// timeout in deciseconds.. that's 10 sec
	ret = tcsetattr(serfd, TCSANOW, &t);
	if (-1 == ret) {
		perror("Failed to set attributes");
		//exit(1);
	}

	return 0;
}

static int fernvale_wait_banner(int fd, const char *banner, int banner_size) {
	//sleep(1);
	//return 0;
	tcdrain(fd);
	uint8_t buf[banner_size];
	int tst;
	int offset = 0;
	int i;

	while (1 == read(fd, &buf[offset], 1)) {

		printf("%c", buf[offset]);

		tst = (offset + 1) % sizeof(buf);
		
		i = 0;
		while (tst != offset) {
			if (banner[i] != buf[tst])
				break;
			tst++;
			i++;
			tst %= sizeof(buf);
		}
		if ((tst == offset) && (banner[i] == buf[tst]))
			return 0;

		offset++;
		offset %= sizeof(buf);
	}
	return -1;
}

static int fernvale_write_stage2(int serfd, int binfd)
{
	struct stat stats;
	uint32_t bytes_left;
	uint32_t bytes_written;
	uint32_t bytes_total;
	int ret;

	if (-1 == fstat(binfd, &stats)) {
		perror("Unable to get file stats");
		exit(1);
	}

	bytes_left = stats.st_size;
	bytes_total = stats.st_size;
	bytes_written = 0;
	printf("%d bytes... ", bytes_total);
	fflush(stdout);
	{
		uint8_t b;

		b = bytes_total & 0xff;
		write(serfd, &b, 1);

		b = bytes_total >> 8 & 0xff;
		write(serfd, &b, 1);

		b = bytes_total >> 16 & 0xff;
		write(serfd, &b, 1);

		b = bytes_total >> 24 & 0xff;
		write(serfd, &b, 1);
	}

#if (STAGE_2_WRITE_ALL_AT_ONCE)
	char bfr[bytes_total];
	ret = read(binfd, bfr, sizeof(bfr));
	if (-1 == ret) {
		perror("Unable to read data from payload file");
		return -1;
	}
	else if (ret != bytes_total) {
		fprintf(stderr, "Shortened read (want: %d got: %d)\n",
				bytes_total, ret);
		return -1;
	}

	ret = write(serfd, bfr, sizeof(bfr));
	if (-1 == ret) {
		perror("Unable to write data to output");
		return -1;
	}
	else if (ret != bytes_total) {
		fprintf(stderr, "Shortened write (want: %d got: %d)\n",
				(int) sizeof(bfr), ret);
		return -1;
	}
	bytes_written = ret;
#else /* ! STAGE_2_WRITE_ALL_AT_ONCE */
	char bfr[STAGE_2_WRITE_SIZE];
	while (bytes_left) {
		uint32_t bytes_to_write;
		
		bytes_to_write = sizeof(bfr);
		if (bytes_left < bytes_to_write)
			bytes_to_write = bytes_left;
		
		ret = read(binfd, bfr, bytes_to_write);
		if (-1 == ret) {
			perror("Unable to read data from payload file");
			return -1;
		}
		else if (ret != bytes_to_write) {
			fprintf(stderr, "Shortened read (want: %d got: %d)\n",
					bytes_to_write, ret);
			return -1;
		}

		ret = write(serfd, bfr, bytes_to_write);
		if (-1 == ret) {
			perror("Unable to write data to output");
			return -1;
		}
		else if (ret != bytes_to_write) {
			fprintf(stderr, "Shortened write (want: %d got: %d)\n",
					bytes_to_write, ret);
			return -1;
		}

		bytes_left    -= bytes_to_write;
		bytes_written += bytes_to_write;

		printf("%6d / %6d\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
				bytes_written, bytes_total);
		fflush(stdout);
	}
#endif /* ! STAGE_2_WRITE_ALL_AT_ONCE */

	printf("%6d / %6d ", bytes_written, bytes_total);
	tcdrain(serfd);

	return 0;
}

static int fernvale_write_stage3(int serfd, int binfd)
{
	struct stat stats;
	uint32_t bytes_left, bytes_written, bytes_total;
	int ret;
	char cmd[128];

	if (-1 == fstat(binfd, &stats)) {
		perror("Unable to get file stats");
		exit(1);
	}

	bytes_left = snprintf(cmd, sizeof(cmd) - 1,
			"loadjmp 0 %d\n", (int)stats.st_size);
	write(serfd, cmd, bytes_left);
	read(serfd, cmd, sizeof(cmd));

	bytes_left = stats.st_size;
	bytes_total = stats.st_size;
	bytes_written = 0;

	printf("%d bytes... ", bytes_total);
	fflush(stdout);

#if (STAGE_3_WRITE_ALL_AT_ONCE)
	char bfr[bytes_total];
	ret = read(binfd, bfr, sizeof(bfr));
	if (-1 == ret) {
		perror("Unable to read data from payload file");
		return -1;
	}
	else if (ret != bytes_total) {
		fprintf(stderr, "Shortened read (want: %d got: %d)\n",
				bytes_total, ret);
		return -1;
	}

	ret = write(serfd, bfr, sizeof(bfr));
	if (-1 == ret) {
		perror("Unable to write data to output");
		return -1;
	}
	else if (ret != bytes_total) {
		fprintf(stderr, "Shortened write (want: %d got: %d)\n",
				(int) sizeof(bfr), ret);
		return -1;
	}
	bytes_written = ret;
#else /* ! STAGE_3_WRITE_ALL_AT_ONCE */
	char bfr[STAGE_3_WRITE_SIZE];
	while (bytes_left) {
		uint32_t bytes_to_write;
		
		bytes_to_write = sizeof(bfr);
		if (bytes_left < bytes_to_write)
			bytes_to_write = bytes_left;
		
		ret = read(binfd, bfr, bytes_to_write);
		if (-1 == ret) {
			perror("Unable to read data from payload file");
			return -1;
		}
		else if (ret != bytes_to_write) {
			fprintf(stderr, "Shortened read (want: %d got: %d)\n",
					bytes_to_write, ret);
			return -1;
		}

		ret = write(serfd, bfr, bytes_to_write);
		if (-1 == ret) {
			perror("Unable to write data to output");
			return -1;
		}
		else if (ret != bytes_to_write) {
			fprintf(stderr, "Shortened write (want: %d got: %d)\n",
					bytes_to_write, ret);
			return -1;
		}

		bytes_left    -= bytes_to_write;
		bytes_written += bytes_to_write;

		printf("%6d / %6d\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
				bytes_written, bytes_total);
		fflush(stdout);
	}
#endif /* ! STAGE_3_WRITE_ALL_AT_ONCE */

	printf("%6d / %6d ", bytes_written, bytes_total);
	tcdrain(serfd);

	return 0;
}

static void test_begin(const char *msg) {
	printf("    %s: ", msg);
	fflush(stdout);
}

static void test_end(int success) {
	if (!success)
		printf("Ok\n");
	else
		printf("Failed (%d)\n", success);
	fflush(stdout);
}

static void test_fail(const char *msg) {
	printf("Failed: %s\n", msg);
	fflush(stdout);
}

static uint32_t chr_to_keymask(uint8_t c) {
	if ((c >= '0') && (c <= '9'))
		return 1 << (c - '0');
	if (c == 'L')
		return (1 << 10);
	if (c == 'R')
		return (1 << 11);
	if (c == 'U')
		return (1 << 12);
	if (c == 'D')
		return (1 << 13);
	if (c == 'A')
		return (1 << 14);
	if (c == 'B')
		return (1 << 15);
	if (c == '*')
		return (1 << 16);
	if (c == '#')
		return (1 << 17);
	return 0;
}

static void  putpixel(uint16_t *s, uint32_t x, uint32_t y, uint16_t c) {
	if (x > 240)
		return;
	if (y > 320)
		return;
	s[y * 240 + x] = c;
}

static void draw_circle(uint16_t *screen, int x, int y, int radius, uint16_t color) {
	int a, b, P;

	// Calculate intermediates
	a = 1;
	b = radius;
	P = 4 - radius;

	// Away we go using Bresenham's circle algorithm
	// Optimized to prevent double drawing
	putpixel(screen, x, y + b, color);
	putpixel(screen, x, y - b, color);
	putpixel(screen, x + b, y, color);
	putpixel(screen, x - b, y, color);
	do {
		putpixel(screen, x + a, y + b, color);
		putpixel(screen, x + a, y - b, color);
		putpixel(screen, x + b, y + a, color);
		putpixel(screen, x - b, y + a, color);
		putpixel(screen, x - a, y + b, color);
		putpixel(screen, x - a, y - b, color);
		putpixel(screen, x + b, y - a, color);
		putpixel(screen, x - b, y - a, color);
		if (P < 0)
			P += 3 + 2*a++;
		else
			P += 5 + 2*(a++ - b--);
	} while(a < b);
	putpixel(screen, x + a, y + b, color);
	putpixel(screen, x + a, y - b, color);
	putpixel(screen, x - a, y + b, color);
	putpixel(screen, x - a, y - b, color);
}

static void draw_circle_filled(uint16_t *s, int x, int y, int r, uint16_t c) {
	int i;

	for (i = 0; i < r; i++)
		draw_circle(s, x, y, i, c);
}

static void draw_button(uint16_t *s, int x, int y, int c, uint32_t keymask, uint8_t k) {
	if (keymask & chr_to_keymask(k))
		draw_circle_filled(s, x, y, 16, c);
	else
		draw_circle(s, x, y, 16, c);
}

static void update_screen_bitmap(uint16_t *screen, uint32_t keymask) {

	int i;
	uint16_t color = 0xffff;

	memset(screen, 0, 320 * 240 * 2);

	draw_button(screen, 64, 32, color, keymask, 'A');
	draw_button(screen, 192, 32, color, keymask, 'B');

	draw_button(screen, 128, 64 + 8, color, keymask, 'U');
	draw_button(screen, 64,  80 + 8, color, keymask, 'L');
	draw_button(screen, 192, 80 + 8, color, keymask, 'R');
	draw_button(screen, 128, 96 + 8, color, keymask, 'D');

	draw_button(screen, 64,  128 + 16, color, keymask, '1');
	draw_button(screen, 128, 128 + 16, color, keymask, '2');
	draw_button(screen, 196, 128 + 16, color, keymask, '3');

	draw_button(screen, 64,  176 + 16, color, keymask, '4');
	draw_button(screen, 128, 176 + 16, color, keymask, '5');
	draw_button(screen, 196, 176 + 16, color, keymask, '6');

	draw_button(screen, 64,  224 + 16, color, keymask, '7');
	draw_button(screen, 128, 224 + 16, color, keymask, '8');
	draw_button(screen, 196, 224 + 16, color, keymask, '9');

	draw_button(screen, 64,  272 + 16, color, keymask, '*');
	draw_button(screen, 128, 272 + 16, color, keymask, '0');
	draw_button(screen, 196, 272 + 16, color, keymask, '#');
}

static void draw_bitmap_to_screen(int serfd, void *bitmap, int size) {
	char cmd[128];
	uint32_t count;

	fernvale_wait_banner(serfd, prompt, strlen(prompt));
	count = snprintf(cmd, sizeof(cmd) - 1, "load 0x40000 %d\n", size);
	write(serfd, cmd, count);
	read(serfd, cmd, count);
	write(serfd, bitmap, size);

	fernvale_wait_banner(serfd, prompt, strlen(prompt));
	count = snprintf(cmd, sizeof(cmd) - 1, "lcd run\n");
	write(serfd, cmd, count);
	read(serfd, cmd, count);
}

static void do_factory_test(int serfd) {
	char cmd[128];
	uint32_t count;
	int ret;
	struct termios t;
	int light_is_on = 1;

	/*
	ret = tcgetattr(serfd, &t);
	if (-1 == ret) {
		perror("Failed to get attributes");
		exit(1);
	}
	cfmakeraw(&t);
	ret = tcsetattr(serfd, TCSANOW, &t);
	if (-1 == ret) {
		perror("Failed to set attributes");
		exit(1);
	}
	*/

	/* Factory test plan:
	 *   - Drive the LCD
	 *   - Flash both LEDs (mainboard and breakout)
	 *   - Register keypresses
	 */
	printf("\n");

	test_begin("Turn on LED");
	fernvale_wait_banner(serfd, prompt, strlen(prompt));
	count = snprintf(cmd, sizeof(cmd) - 1, "led 1\n");
	write(serfd, cmd, count);
	read(serfd, cmd, count);
	test_end(0);

	test_begin("Keypad");
	{
		uint16_t screen_bitmap[320 * 240];
		memset(screen_bitmap, 0, sizeof(screen_bitmap));

		uint32_t keymask = 0x3ffff;
		int needs_to_rerun = 1;
		uint8_t c;

		update_screen_bitmap(screen_bitmap, keymask);
		draw_bitmap_to_screen(serfd, screen_bitmap, sizeof(screen_bitmap));
		while (keymask) {

			if (needs_to_rerun) {
				fernvale_wait_banner(serfd, prompt, strlen(prompt));
				count = snprintf(cmd, sizeof(cmd) - 1, "keypad 1\n");
				write(serfd, cmd, count);
				read(serfd, cmd, count);
	//				printf("Getting command back: ");
				fflush(stdout);
				do {
					ret = read(serfd, cmd, 1);
	//					printf("%c [%x] %d", cmd[0], cmd[0], ret);
					//printf("%c", cmd[0]);
					fflush(stdout);
				} while ((ret == 1) && (cmd[0] != '\n'));

	//				printf("\n\nWaiting for first message: ");
				fflush(stdout);
				do {
					ret = read(serfd, cmd, 1);
	//					printf("%c [%x] %d", cmd[0], cmd[0], ret);
					//printf("%c", cmd[0]);
	//					fflush(stdout);
				} while ((ret == 1) && (cmd[0] != '\n'));

	//				printf("\n\nWaiting for second message: ");
				fflush(stdout);
				do {
					ret = read(serfd, cmd, 1);
	//					printf("%c [%x] %d", cmd[0], cmd[0], ret);
					//printf("%c", cmd[0]);
	//					fflush(stdout);
				} while ((ret == 1) && (cmd[0] != '\n'));

	//				printf("\n\nMonitoring keypresses\n");
				needs_to_rerun = 0;
			}

			if (read(serfd, &c, sizeof(c)) != 1) {
				test_fail("Unable to read from port");
				return;
			}

			printf("\nGot key: %d (%c)", c, c);
			fflush(stdout);
			keymask &= ~chr_to_keymask(c);
			printf("Keymask: 0x%5x\n", keymask);

			if (keymask)
				needs_to_rerun = 1;
			else if (!keymask) {
				cmd[0] = '\n';
				write(serfd, cmd, 1);
			}

			update_screen_bitmap(screen_bitmap, keymask);
			draw_bitmap_to_screen(serfd, screen_bitmap, sizeof(screen_bitmap));

			if (light_is_on) {
				fernvale_wait_banner(serfd, prompt, strlen(prompt));
				count = snprintf(cmd, sizeof(cmd) - 1, "led 0\n");
				write(serfd, cmd, count);
				read(serfd, cmd, count);
				light_is_on = 0;
			}
		}
		/* Exit test */
		count = snprintf(cmd, sizeof(cmd) - 1, "\n");
		write(serfd, cmd, count);
		read(serfd, cmd, sizeof(cmd));
		test_end(0);
	}
	return;
}

static void cmd_begin(const char *msg) {
	printf("%s... ", msg);
	fflush(stdout);
}

static void cmd_end(void) {
	printf("Ok\n");
}

static void cmd_end_fmt(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
}

static void print_help(const char *name)
{
	printf("Usage: %s [-a address] [-l logfile] [-s] [serial port] "
			"[stage 1 bootloader] "
			"[[stage 2 bootloader]] "
			"[payload]\n", name);
	printf("\n");
	printf("Arguments:\n");
	printf("    -a [address]      Set load address for stage 1 bootloader (default: 0x%x)\n",
		FERNLY_USB_LOADER_ADDR);
	printf("    -l [logfile]      Log boot output to the specified file\n");
	printf("    -w                Wait for serial port to appear\n");
	printf("    -s                Enter boot shell\n");
	printf("    -t                Run fernly factory test\n");
	printf("    -h                Print this help\n");
	printf("\n");
	printf("If you don't want a stage 2 bootloader, you may omit "
		"it, and this program will jump straight to the "
		"payload, loaded at offset 0x%08x.\n",
		FERNLY_USB_LOADER_ADDR);
	printf("\n");
	printf("The boot shell allows you to interact directly with the stage "
	       "2 bootloader.  If you omit -s, then this program will exit "
	       "after loading either the stage 2 bootloader or the payload.\n");
}

int main(int argc, char **argv) {
	int serfd, binfd = -1, s1blfd, payloadfd = -1, logfd = -1;
	char *logname = NULL;
	uint32_t ret;
	int opt;
	int shell = 0;
	int wait_serial = 0;
	int factory_test = 0;

	uint32_t usb_loader_addr = FERNLY_USB_LOADER_ADDR;

	while ((opt = getopt(argc, argv, "a:hl:swt")) != -1) {
		switch(opt) {

		case 'a':
			usb_loader_addr = strtoul(optarg, NULL, 0);
			break;

		case 'l':
			logname = strdup(optarg);
			break;

		case 's':
			shell = 1;
			break;

		case 'w':
			wait_serial = 1;
			break;

		case 't':
			factory_test = 1;
			break;

		default:
		case 'h':
			print_help(argv[0]);
			return 1;
			break;

		}
	}

	argc -= (optind - 1);
	argv += (optind - 1);

	if ((argc != 3) && (argc != 4) && (argc != 5)) {
                printf("%d is wrong # of args\n", argc);
		exit(1);
	}

	if (wait_serial) {
		printf("Waiting for serial port to connect: .");
		fflush(stdout);
	}
	while (1) {
		serfd = open(argv[1], O_RDWR);
		if (-1 == serfd) {
			if (wait_serial) {
				printf(".");
				fflush(stdout);
				sleep(1);
				continue;
			} else {
				perror("Unable to open serial port");
				exit(1);
			}
		}
		break;
	}
	if (wait_serial) {
		printf("\n");
	}

	s1blfd = open(argv[2], O_RDONLY);
	if (-1 == s1blfd) {
		perror("Unable to open stage 1 bootloader");
		exit(1);
	}

	if (argc == 4 || argc == 5) {
		binfd = open(argv[3], O_RDONLY);
		if (-1 == binfd) {
			perror("Unable to open firmware file");
			exit(1);
		}
	}

	if (argc == 5) {
		payloadfd = open(argv[4], O_RDONLY);
		if (-1 == payloadfd) {
			perror("Unable to open payload file");
			exit(1);
		}
	}

	cmd_begin("Setting serial port parameters");
	ASSERT(fernvale_set_serial(serfd));
	cmd_end();

	cmd_begin("Initiating communication");
	ASSERT(fernvale_hello(serfd));
	cmd_end();

	cmd_begin("Getting hardware version");
	cmd_end_fmt("0x%04x", fernvale_read_reg16(serfd, mtk_config_offset));

	cmd_begin("Getting chip ID");
	cmd_end_fmt("0x%04x", fernvale_read_reg16(serfd, mtk_config_offset + 8));

	cmd_begin("Getting boot config (low)");
	cmd_end_fmt("0x%04x", fernvale_read_reg16(serfd, 0xa0000000 + 0x10));

	cmd_begin("Getting boot config (high)");
	cmd_end_fmt("0x%04x", fernvale_read_reg16(serfd, 0xa0000000 + 0x14));

	cmd_begin("Getting hardware subcode");
	cmd_end_fmt("0x%04x", fernvale_read_reg16(serfd, mtk_config_offset + 12));

	cmd_begin("Getting hardware version (again)");
	cmd_end_fmt("0x%04x", fernvale_read_reg16(serfd, mtk_config_offset));

	cmd_begin("Getting chip firmware version");
	cmd_end_fmt("0x%04x", fernvale_read_reg16(serfd, mtk_config_offset + 4));

	cmd_begin("Getting security version");
	cmd_end_fmt("v %d", fernvale_security_version(serfd));
	
	cmd_begin("Enabling security (?!)");
	fernvale_send_do_security(serfd);
	cmd_end();

	cmd_begin("Reading ME");
	fernvale_print_me(serfd);

	cmd_begin("Disabling WDT");
	fernvale_write16(serfd, 0xa0030000, 0x2200);
	cmd_end();

	cmd_begin("Disabling Battery WDT");
	fernvale_write16(serfd, 0xa0700a24, 2);
	cmd_end();

	cmd_begin("Reading RTC Baseband Power Up (0xa0710000)");
	ret = fernvale_read16(serfd, 0xa0710000);
	cmd_end_fmt("0x%04x", ret);

	cmd_begin("Reading RTC Power Key 1 (0xa0710050)");
	ret = fernvale_read16(serfd, 0xa0710050);
	cmd_end_fmt("0x%04x", ret);

	cmd_begin("Reading RTC Power Key 2 (0xa0710054)");
	ret = fernvale_read16(serfd, 0xa0710054);
	cmd_end_fmt("0x%04x", ret);

	/* Program RTC */
	cmd_begin("Setting seconds");
	fernvale_write16(serfd, 0xa0710010, 0);
	cmd_end();

	cmd_begin("Disabling alarm IRQs");
	fernvale_write16(serfd, 0xa0710008, 0);
	cmd_end();

	cmd_begin("Disabling RTC IRQ interval");
	fernvale_write16(serfd, 0xa071000c, 0);
	cmd_end();

	cmd_begin("Enabling transfers from core to RTC");
	fernvale_write16(serfd, 0xa0710074, 1);
	cmd_end();

	cmd_begin("Reading RTC Baseband Power Up (0xa0710000)");
	ret = fernvale_read16(serfd, 0xa0710000);
	cmd_end_fmt("0x%04x", ret);

	cmd_begin("Getting security configuration");
	fernvale_print_sec_conf(serfd);

	cmd_begin("Getting PSRAM mapping");
	ret = fernvale_read32(serfd, 0xa0510000);
	cmd_end_fmt("0x%04x", ret);

	cmd_begin("Disabling PSRAM -> ROM remapping");
	fernvale_write32(serfd, 0xa0510000, 2);
	usleep(20000);
	cmd_end();

	cmd_begin("Checking PSRAM mapping");
	ret = fernvale_read32(serfd, 0xa0510000);
	cmd_end_fmt("0x%04x", ret);

	cmd_begin("Checking on PSRAM mapping again");
	ret = fernvale_read32(serfd, 0xa0510000);
	cmd_end_fmt("0x%04x", ret);

	cmd_begin("Updating PSRAM mapping again for some reason");
	fernvale_write32(serfd, 0xa0510000, 2);
	usleep(50000);
	cmd_end();

	cmd_begin("Reading some fuses");
	fernvale_send_cmd(serfd, 0xd8);
	cmd_end_fmt("0x%08x", fernvale_get_int32(serfd));
	fernvale_get_int16(serfd);

	cmd_begin("Enabling UART");
	ASSERT(fernvale_send_cmd(serfd, 0xdc));
	fernvale_send_int32(serfd, 115200);
	ASSERT(ret = fernvale_get_int16(serfd));
	cmd_end_fmt("0x%04x", ret);

	cmd_begin("Loading Fernly USB loader");
	ASSERT(fernvale_cmd_send_fd(serfd, usb_loader_addr, s1blfd));
	cmd_end();

	cmd_begin("Executing Ferly USB loader");
	ASSERT(fernvale_cmd_jump(serfd, usb_loader_addr));
	cmd_end();

	cmd_begin("Waiting for Fernly USB loader banner");
	ASSERT(fernvale_wait_banner(serfd, ">", 1));
	cmd_end();

	if (binfd == -1) {
		close(serfd);
		return 0;
	}

	if (binfd != -1) {
		cmd_begin("Writing stage 2");
		ASSERT(fernvale_write_stage2(serfd, binfd));
		cmd_end();
	}

	if (factory_test) {
		cmd_begin("Starting factory test");
		do_factory_test(serfd);
		cmd_end();
		return 0;
	}

	if (payloadfd != -1) {
		cmd_begin("Entering download mode");
		fernvale_wait_banner(serfd, prompt, strlen(prompt));
		cmd_end();
		
		cmd_begin("Writing payload");
		fernvale_write_stage3(serfd, payloadfd);
		cmd_end();
	}

	if (shell) {
		cmd_begin("Start Fernly shell");
		uint8_t bfr;
		int ret;
		struct termios t;
		ret = tcgetattr(1, &t);
		if (-1 == ret) {
			perror("Failed to get attributes");
			exit(1);
		}
		old_termios = t;
		atexit(restore_termios);
		cfmakeraw(&t);
		ret = tcsetattr(1, TCSANOW, &t);
		if (-1 == ret) {
			perror("Failed to set attributes");
			//exit(1);
		}

		if (logname) {
			logfd = open(logname, O_WRONLY | O_CREAT | O_APPEND);
			if (-1 == logfd)
				perror("Warning: could not open logfile");
		}

		while (1) {
			fd_set rfds;
			FD_ZERO(&rfds);

			FD_SET(serfd, &rfds);
			FD_SET(STDIN_FILENO, &rfds);

			select(serfd + 1, &rfds, NULL, NULL, NULL);

			if (FD_ISSET(serfd, &rfds)) {
				if (1 != read(serfd, &bfr, sizeof(bfr)))
					break;
				if (bfr == 0x7f) {
					char txt[] = " \b";
					write(STDOUT_FILENO, txt, sizeof(txt));
				}
				else {
					write(STDOUT_FILENO, &bfr, sizeof(bfr));
					if (logfd != -1)
						write(logfd, &bfr, sizeof(bfr));
				}
			}
			if (FD_ISSET(STDIN_FILENO, &rfds)) {
				if (1 != read(STDIN_FILENO, &bfr, sizeof(bfr)))
					break;
				if (bfr == 0x3)
					break;
				write(serfd, &bfr, sizeof(bfr));
			}
		}
	}
	else {
		cmd_begin("Waiting for ready prompt");
		fernvale_wait_banner(serfd, prompt, strlen(prompt));
		cmd_end();
	}

	close(serfd);
	
	return 0;
}
