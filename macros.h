#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>

// idk why i made any of these macros now that i think about it, maybe to get rid of magic numbers,
// but there's still a ton of magic numbers everywhere
#define CHUNKSIZE 65536
#define BYTE 256
#define MAX 200

// more dumb things
#define C2_NEIGHBOURS_END 0x1B4
#define C2_NEIGHBOURS_FLAGS_END 0x1D4

// commands
#define KILL 2089250961u
#define HELP 2089138798u
#define EXPORT 2939723207u
#define EXPORTALL 1522383616u
#define CHANGEPRINT 2239644728u
#define CHANGEMODE 588358864u
#define STATUS 3482341513u
#define WIPE 2089682330u
#define IMPORT 3083219648u
#define INTRO 223621809
#define RESIZE 3426052343

//functional prototypes, also list of functions excluding main and main1
void resize_chunk_handler(unsigned char *chunk);
void resize_entity(unsigned char *item, int itemsize, double scale[3]);
void resize_zone(int fsize, unsigned char *buffer, double scale[3]);
void resize_level(FILE *fpath, char *path, double scale[3], char *time);
void resize_folder(DIR *df, char *path, double scale[3], char *time);
void resize_scenery(int fsize, unsigned char *buffer, double scale[3]);
void resize_main(char *time);
void chunksave(unsigned char *chunk, int *index, int *curr_off, int *curr_chunk, FILE *fnew, int offsets[]);
int camfix(unsigned char *cam, int length);
void entitycoordfix(unsigned char *item, int itemlength);
unsigned int from_u32(unsigned char *data);
void scenery(unsigned char *buffer, int entrysize,char *lvlid, char *date);
unsigned int nsfChecksum(const unsigned char *data);
int filelister(char *path, FILE *base);
int import(char *time);
void printstatus(int zonetype, int gamemode, int portmode);
void clrscr();
void askmode(int *zonetype);
void condprint(char *s, int print_en, FILE *flog);
void askprint(int *print_en);
void generic_entry(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void gool(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void zone(unsigned char *buffer, int entrysize,char *lvlid, char *date, int zonetype);
void model(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void countwipe(int counters[22]);
void countprint(int counters[22], int gamemode, int portmode, char *temp, int print_en, FILE *flog);
void animation(unsigned char *buffer, int entrysize,char *lvlid, char *date);
void pathstring(char *finalpath, char *type, int eid, char *lvlid, char *date);
void eid_conv(unsigned int m_value, char *eid);
int normal_chunk(unsigned char *buffer, char *lvlid, char *date, int zonetype);
int texture_chunk(unsigned char *buffer, char *lvlid, char *date);
int sound_chunk(unsigned char *buffer);
int wavebank_chunk(unsigned char *buffer);
int chunk_handler(unsigned char *buffer,int chunkid, char *lvlid, char *date, int zonetype);
int exprt(int zone, char *fpath, char *date);
void intro_text();
void print_help();
const unsigned long hash(const char *str);

