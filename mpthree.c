#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef unsigned char byte; // used for bytes (obvs)
typedef char u8;   // used for low integers 

struct frame {
	uint32_t header;
	byte* bytes;
	size_t size;
};

bool has_id3_metadata(byte* _bytes) {
	uint32_t metadata = (_bytes[0] << 16) | (_bytes[1] << 8) | (_bytes[2]);
	if (metadata == 0x494433) return true;
	return false;
}

uint32_t read_frame_header(byte* _bytes) {
	// printf("%X\n", _bytes[1] << 16);
	uint32_t word = 0;
	word |= ((_bytes[0] << 24) & 0xFF000000);
	word |= ((_bytes[1] << 16) & 0xFF0000);
	word |= ((_bytes[2] << 8) & 0xFF00);
	word |= (_bytes[3] & 0xFF);
	return word;
}

/* checks if the ptr is at the start of the header */
/* 0xfff is indicator for the frame header */
bool soframe(byte* bytes) {
	uint16_t word = 0;
	word |= (bytes[0] << 8) & 0xFF00;
	word |= (bytes[1] & 0xE0);
	if (word == 0xFFE0) return true;
	return false;
}

/* sets the bytes ptr to the start of the next frame header */
/* also returns false if there is no next header */
bool stonframe(byte** bytes, byte* end) {
	while (!soframe(*bytes)) {
		if (*bytes == end || (*bytes + 3) >= end)
			return false;
		(*bytes)++;
	}
	return true;
}

struct frame* readframes(byte* fbytes, size_t size, size_t* frame_count) {
	struct frame* _frames = (struct frame*)malloc(sizeof(struct frame));
	byte* _end = fbytes + size;
	byte* _start_frame = fbytes;
	while (stonframe(&fbytes, _end)) {
		if (_start_frame == fbytes) {
			fbytes++;
			continue;
		}
		_frames[*frame_count].header = read_frame_header(_start_frame);
		_frames[*frame_count].bytes = _start_frame;
		_frames[*frame_count].size = (size_t)(fbytes - _start_frame);
		_start_frame = fbytes;
		fbytes += 4;
		_frames = realloc(_frames, (++(*frame_count) + 1) * sizeof(struct frame));
	}
	return _frames;
}

size_t readbytes(const char* _path, byte** _bytes);
void printusage(void);
void printhex(const byte* _bytes, size_t size);

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printusage();
		return -1;
	}

	const char* path = argv[1];
	byte* fbytes = NULL;
	size_t size = readbytes(path, &fbytes);
	byte* start = fbytes;

	stonframe(&fbytes, fbytes + size);
	
	size_t frame_count = 0;
	struct frame* mp3_frames = readframes(fbytes, size, &frame_count);
	
	for (size_t i = 0; i < frame_count; i++) {
		printf("HEADER = %X\nSIZE = %zX\n", mp3_frames[i].header, mp3_frames[i].size);
	}

	return 0;
}

void printhex(const byte* _bytes, size_t size) {
	for (size_t i = 1; i <= size; i++) {
		printf("%#02x ", *_bytes);
		if (!(i % 8)) {
			printf("\n");
		}
		else if (!(i % 4)) {
			printf("  ");
		}
		_bytes++;
	}
}

void printusage(void) {
	printf("mpthree by m1ad3n\n");
	printf("usage: mpthree <audio-file>\n");
}

size_t readbytes(const char* _path, byte** _bytes) {
	FILE* fptr = fopen(_path, "rb");
	if (!fptr) {
		fprintf(stderr, "mpthree: fopen failed\n");
		return 0;
	}

	// find out the file size
	fseek(fptr, 0, SEEK_END);
	size_t filesize = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	// allocate the memory
	*_bytes = (byte*)malloc(filesize + 1);
	(*_bytes)[filesize] = '\0';

	// read the data
	fread(*_bytes, 1, filesize, fptr);
	fclose(fptr);

	return filesize;
}

