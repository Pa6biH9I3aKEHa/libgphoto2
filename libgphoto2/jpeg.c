/* This code was written by Nathan Stenzel for gphoto */
/* GPL */

#include <stdio.h>
#include "jpeg.h"

void chunk_new(struct chunk *mychunk, int length)
{
    mychunk->size=length;
//    printf("New chunk of size %i\n", mychunk->size);
    mychunk->data = (char *)malloc(length);
}

void chunk_print(struct chunk *mychunk)
{
    int x;
//    printf("Size=%i\n", mychunk->size);
    for (x=0; x<mychunk->size; x++)
        printf("%hX ", mychunk->data[x]);
    printf("\n");
}

void chunk_destroy(struct chunk *mychunk)
{
    mychunk->size=0;
    free(mychunk->data);
}

void jpeg_init(struct jpeg *myjpeg)
{
    myjpeg->count=0;
}

void jpeg_destroy(struct jpeg *myjpeg)
{
    int count;
    for (count=0; count<myjpeg->count; count++)
        chunk_destroy(&(myjpeg->marker[count]));
    myjpeg->count=0;
}

char jpeg_findff(int *location, struct chunk *picture) {
//printf("Entered jpeg_findamarker!!!!!!\n");
    while(*location<picture->size)
    {
        printf("%hX ",picture->data[*location]);
        if (picture->data[*location]==0xff)
        {
//        printf("return success!\n");
        return 1;
        }
    (*location)++;
    }
//    printf("return failure!\n");
    return 0;
}

char jpeg_findactivemarker(char *id, int *location, struct chunk *picture) {
//    printf("Entered jpeg_findactivemarker!!!!!!!!\n");
    while(jpeg_findff(location, picture) && ((*location+1)<picture->size))
        if (picture->data[*location+1]) {
            *id=picture->data[*location+1];
            return 1;
        }
    return 0;
}

void jpeg_add_marker(struct jpeg *myjpeg, struct chunk *picture, int start, int end)
{
    int length;
    length=(int)(end-start+1);
//    printf("Add marker #%i starting from %i and ending at %i for a length of %i\n", myjpeg->count, start, end, length);
    chunk_new(&(myjpeg->marker[myjpeg->count]), length);
//    printf("Read length is: %i\n", myjpeg->marker[myjpeg->count].size);
    memcpy(myjpeg->marker[myjpeg->count].data, picture->data+start, length);
    chunk_print(&(myjpeg->marker[myjpeg->count]));
    myjpeg->count++;
}

char *jpeg_markername(int c)
{
    int x;
//    printf("searching for marker %X in list\n",c);
//    printf("%i\n", sizeof(markers));
    for (x=0; x<sizeof(markers); x++)
    {
//        printf("checking to see if it is marker %X\n", markers[x]);

        if (c==markers[x])
            return (char *)markernames[x];
    }
    return "Undefined marker";
}

void jpeg_parse(struct jpeg *myjpeg, struct chunk *picture)
{
    int position=0;
    int lastposition;
    char id;
    if (picture->data[0]!=0xff)
        {
            jpeg_findactivemarker(&id, &position, picture);
            jpeg_add_marker(myjpeg,picture,0,position-1);
            lastposition=position;
            position=position++;
        }
        else
        {
        lastposition=position;
        position+=2;
        jpeg_findactivemarker(&id, &position, picture);
        jpeg_add_marker(myjpeg,picture,0,position-1);
        lastposition=position;
        position+=2;
        }
    while (position<picture->size)
        {
            jpeg_findactivemarker(&id, &position, picture);
            jpeg_add_marker(myjpeg,picture,lastposition,position-1);
            lastposition=position;
            position+=2;
        }
    position-=2;
    if (position<picture->size)
    jpeg_add_marker(myjpeg,picture,lastposition,picture->size-1);
}

void jpeg_print(struct jpeg *myjpeg)
{
int c;
printf("There are %i markers\n", myjpeg->count);
for (c=0; c < myjpeg->count; c++)
    {
    printf("%s:\n",jpeg_markername(myjpeg->marker[c].data[1]));
    chunk_print(&myjpeg->marker[c]);
    }
}

/* TEST CODE SECTION */
/*
char testdata[] ={
    0xFF,0xD8, 0xFF,0xE0, 0xff,0xDB, 0xFF,0xC4, 0xFF,0xDA,
    0xFF,0xC0, 0xff,0xff};

int main()
{
struct chunk picture;
struct jpeg myjpeg;
char id;
int location=0,x;
jpeg_init(&myjpeg);
picture.data=testdata;
picture.size=sizeof(testdata);
printf("testdata size is %i\n",picture.size);

printf("Call jpeg_parse!!!!!!!!!!!!!!!!!!!!!!!\n");
jpeg_parse(&myjpeg,&picture);

printf("\nPrint the jpeg table\n");
jpeg_print(&myjpeg);
printf("chunk_new and chunk_destroy tests\n");
chunk_new(&picture,10);
for (x=0; x<10; x++) picture.data[x]=x;
for (x=0; x<10; x++) printf("%hX ",picture.data[x]);
chunk_destroy(&picture);
printf("\n");
}
*/

