/**
 * BMP output and input. 24 bit colour only.
 * Stateful and terrible. Less than thread-safe. Use for horrible hacks only.
 *
 * (c) L. Diener 2010
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

FILE* bmp_file;
int line_pos = 0;
int line_max = 0;

///////// Header data /////////

#pragma pack (1)
typedef struct bmp_header {
	uint8_t file_type[ 2 ]; // "BM"
	uint32_t file_size;
	uint16_t reserved_1; // 0
	uint16_t reserved_2; // 0
	uint32_t pixel_offset; // 54
	uint32_t header_size; // 40
	uint32_t x_size;
	uint32_t y_size;
	uint16_t planes; // 1
	uint16_t bpp; // 24
	uint32_t compression; // 0
	uint32_t image_size; // 0
	uint32_t x_ppm; // 0
	uint32_t y_ppm; // 0
	uint32_t used_colors; // 0
	uint32_t important_colors; // 0
} bmp_header;

///////// Writing /////////

// Write BMP header
void bmp_init(const char* file_name, int x_size, int y_size) {
	// Make us a header.
	bmp_header file_head;
	file_head.file_type [ 0 ] = 'B';
	file_head.file_type [ 1 ] = 'M';
	file_head.file_size = x_size * y_size * 3 + 54;
	file_head.reserved_1 = 0;
	file_head.reserved_2 = 0;
	file_head.pixel_offset = 54;
	file_head.header_size = 40;
	file_head.x_size = x_size;
	file_head.y_size = y_size;
	file_head.planes = 1;
	file_head.bpp = 24;
	file_head.compression = 0;
	file_head.image_size = 0;
	file_head.x_ppm = 0;
	file_head.y_ppm = 0;
	file_head.used_colors = 0;
	file_head.important_colors = 0;

	// Write file header.
	bmp_file = fopen(file_name, "w");
	fwrite((char*)(&file_head), 1, sizeof(file_head), bmp_file);

	line_max = x_size * 3;
	line_pos = 0;
}

// Write a single pixel. r, g and b between 0 and 255.
void bmp_pixel(int r, int g,  int b) {
	char tmp_r = (char)r;
	char tmp_g = (char)g;
	char tmp_b = (char)b;
	fwrite(&tmp_b, 1, 1, bmp_file);
	fwrite(&tmp_g, 1, 1, bmp_file);
	fwrite(&tmp_r, 1, 1, bmp_file);

	line_pos+=3;
	if(line_pos == line_max) {
		while(line_pos % 4 != 0) {
			fwrite(&tmp_b, 1, 1, bmp_file);
			line_pos++;
		}
		line_pos = 0;
	}
}

///////// Reading /////////

// Open a BMP file for reading.
// Write BMP header
void bmp_read(const char* file_name, int* x_size, int* y_size) {
	// Read file header.
	bmp_file = fopen(file_name, "r");
	bmp_header file_head;	
	fread((char*)(&file_head), 1, sizeof(file_head), bmp_file);

	// Set up data for reading
	line_max = file_head.x_size * 3;
	line_pos = 0;
	
	// Return
	*x_size = file_head.x_size;
	*y_size = file_head.y_size;
}

// Read a single BMP pixel. r, g and b between 0 and 255.
void bmp_read_pixel(int* r, int* g,  int* b) {
	unsigned char tmp_r;
	unsigned char tmp_g;
	unsigned char tmp_b;
	fread(&tmp_b, 1, 1, bmp_file);
	fread(&tmp_g, 1, 1, bmp_file);
	fread(&tmp_r, 1, 1, bmp_file);

	line_pos += 3;
	int tmp_x;
	if(line_pos == line_max) {
		while(line_pos % 4 != 0) {
			fread(&tmp_x, 1, 1, bmp_file);
			line_pos++;
		}
		line_pos = 0;
	}
		
	*r = tmp_r;
	*g = tmp_g;
	*b = tmp_b;
}

///////// Et cetera /////////

// Flush file and close.
void bmp_close() {
	fflush(bmp_file);
	fclose(bmp_file);
}
