/*///////////////////////////////////////////////////////////////////////
//                      HANOI TOWER Ver 0.1                            //
//                                                                     //
//                  (C) 1997 by Hwang, Ui Sang                         //
//                                                                     //
//      *Environment : Turbo-C above (BORLAND Inc. classes) for DOS    //
//                     Any memory model                                //
//      *Comment     : Direct access to video memory,                  //
//                            mono(Hercules) : 0xB000                  //
//                            color          : 0xB800                  //
//      *Limit       : You must enter the disk number from 1 to 13.    //
//                     This program runs well on mono, as well as      //
//                     on color.                                       //
//      *Year & date : '97.2.8, '97.2.28, '97.3.7, '97.3.12            //
//      *Note                                                          //
//            Problem => There are three posts and the left most one   //
//                       has disks as many as numbers you entered.     //
//                       You have to move all of the disks to the third//
//                       post one by one and the rule is that any small//
//                       disk can't place over a large one.            //
//                                                                     //
//            Solution=> 1) move disks to post 1 with post 2.          //
//                       2) move disks to post 2 with post 1.          //
//                                                                     //
// ----------------**Diagram of Hanoi Tower Program**----------------  //
//    B.M : post Before Moving                                         //
//    B.U : post Being  Using                                          //
//    A.M : post After  Moving                                         //
//                                                                     //
//                post 0     post 1       post 2                       //
//                 B.M         B.U         A.M                         //
//                  |           |           |   <-- post length        //
//     +---->  0    л           |           |   <-- diskNumber(1~13)   //
//     | +-->  1   ллл          |           |                          //
//     | | +>  2  ллллл         |           |                          //
//     | | |   -----+-----------+-----------+---<-- lower limit        //
//     | | |                                     |- postBM.yCur..(0)   //
//     | | +---- *(disk+0) =   "л\0"   (1)       |- postBU.yCur..(0)   //
//     | +------ *(disk+1) =  "ллл\0"  (3)       |_ postAM.yCur..(0)   //
//     +-------- *(disk+2) = "ллллл\0" (5)                             //
//                   .          .       .           yCur.. increase/   //
//                   .          .       .           decrease when di-  //
//                   .          .       .           sk is moved.       //
///////////////////////////////////////////////////////////////////////*/
#define INLINE_ASM
#define DEBUG_VALUE
#define COORD_ALIGN

#undef INLINE_ASM
#undef DEBUG_VALUE
#undef COORD_ALIGN

#ifdef INLINE_ASM
#pragma inline
#endif

#include <stdarg.h>
#include <dos.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <bios.h>

#ifndef NULL
#define NULL 0
#endif

#ifdef INLINE_ASM
  #define VRamColor   0xB800
  #define VRamMono    0xB000
  int VRAM;
#else
  #define VRamColor   0xB8000000
  #define VRamMono    0xB0000000
  char far * VRAM;
#endif
#define YShift      160
#define XShift      2

#define   F1                         315
#define  _BS                         0x08   /* ASCII code of BackSpace */
#define  _ESC                        0x1b   /* ASCII code of Escape    */
#define  _ENTER                      0x0d   /* ASCII code of Enter     */

#define  InVisible                   0    /* 0 */
#define  UnDerline                   1    /* 1 */
#define  Normal                      7    /* 7 */
#define  BrightUnDerline             9    /* 8 + 1 */
#define  BrightNormal                15   /* 8 + 7 */
#define  Reverse                     112  /* 112   */
#define  BlinkingUnDerline           129  /* 128+1 */
#define  BlinkingNormal              135  /* 128+7 */
#define  BlinkingBrightUnDerline     137  /* 128+8+1 */
#define  BlinkingBrightNormal        143  /* 128+8+7 */
#define  BlinkingReverse             240  /* 128+112 */

#define  LowerLimit                  22
#define  MaximumDiskNumber           20   /* one can run Hanoi.exe with 20 disks */
#define  VerticleLine                179  /*  Г  */
#define  HorizontleLine              196  /*  Ф  */
#define  ReversedT                   193  /*  С  */

				   /*-----------------------------*/
typedef                            /*              +-(hrz)        */
      struct btype {               /*  +-(lft_top) | (rgt_top)-+  */
	     unsigned lft_top;     /*  |           |           |  */
	     unsigned lft_btm;     /*  +->*--------*----*<-----+  */
	     unsigned rgt_top;     /*     | W I N D O W *<--(vrt) */
	     unsigned rgt_btm;     /*  +->*-------------*<---+    */
	     unsigned hrz;         /*  |                     |    */
	     unsigned vrt;         /*  +-(lft_btm) (rgt_btm)-+    */
      }                            /*-----------------------------*/
boxCharacter;

typedef
      struct xyCoord {
	     unsigned secondX;
	     unsigned secondY;
	     unsigned minuteX;
	     unsigned minuteY;
	     unsigned hourX;
	     unsigned hourY;
      }
xyCoordForWatch;

typedef
      struct color {
	     int   second;
	     int   minute;
	     int   hour;
      }
watchColor;

typedef
       struct SPost {
	      int  yCurrentPost;
	      int  postID;
       }
 SPostID;

struct SColor {
	     int post;
	     int title;
	     int message;
	     int error;
	     int disk;
	     int disknumber;
	     int diskcount;
	     int erase;
} SColor;

typedef enum { TRUE = 1, FALSE = 0 } BOOLEAN;

xyCoordForWatch  xyCoordination;    /*/ coordinates for hh/mm/ss/*/
watchColor       watchAttrib;       /*/ each member has color value/*/
SPostID          postBeforeMoving,  /*/ x-coord. for a post before moving/*/
		 postBeingUsing,    /*/ x-coord. for a post, being using/*/
		 postAfterMoving;   /*/ x-coord. for a post after moving/*/

	 int     LengthOfPost, diskNumber, delayTime = 0,
		 noDoubleDelay = 0; /*/ This global veriable is very im-
		 portant because it protects computer from system halt by
		 pressing F1 in vdGetDelay() /*/
unsigned int     prevCursorX, prevCursorY;
unsigned long    count;
unsigned char  **disk;
unsigned int     xCoordOfPost[3] = { 16, 39, 63 };
unsigned char    fillChar[5] = { 176, 177, 178, 219 ,32};
			      /*  А    Б    В    л    */

boxCharacter h1v1 = { 218, 192, 191, 217, 196, 179 };
boxCharacter h2v1 = { 213, 212, 184, 190, 205, 179 };
boxCharacter h1v2 = { 214, 211, 183, 189, 196, 186 };
boxCharacter h2v2 = { 201, 200, 187, 188, 205, 186 };

int   getkey(void);
int   inkey(void);
int   vdGetsHanoi(int x, int y, char *dnumber, int size);
void  acleartextHanoi(int x1, int y1, int x2, int y2, unsigned char fill, int color);
void  boxHanoi(boxCharacter type, int x1, int y1, int x2, int y2, int attr);
void  (*clock_inter)(void);
void  clockRoutine(void);
void  drawBottom(void);
void  initHanoi(void);
void  in_null(void);
void  nputchHanoi( int x, int y, int number, unsigned char munja, int attrib);
void  printfHanoi(int x, int y, int attrib, char *format,...);
void  setClockRoutine(void (*func)());
void  setColorOfWatch(int hour, int min, int sec);
void  setXYCoordOfWatch(int hourX, int hourY, int minX, int minY, int secX, int secY);
void  vdBeginHanoi(void);
void  vdDeleteDisk(void);
void  vdDrawDisk(int nThDisk, SPostID *nThPost);
void  vdEraseDisk(int nThDisk, SPostID *nThPost);
void  vdHanoiTower(int disknumber, SPostID *postBM, SPostID *postBU, SPostID *postAM);
void  vdMoveDisk(int nThDisk, SPostID *postFrom, SPostID *postTo);
void  vdMakeFrame(void);
void  vdMakeDisk(void);
void  vdMessageHanoi(int x, int y, char *message, int attrib);
void  restoretextHanoi(int x1, int y1, int x2, int y2, unsigned char *buff);
void  savetextHanoi(int x1, int y1, int x2, int y2, unsigned char *buff);
void  vdSetDelayTime(void);
void  watch(void);
int   isColor(void);
int   peekabyte(int x, int y);
void  pokeabyte(int x, int y, char abyte, int attr);
void  pokeastring(int x, int y, char *str, int attr);
void  setVRAM(void);
void  saveScreen(void);
void  splitScreen(void);
void  mergeScreen(void);

/*-----------------------------------------------------------------------*/

void main(void)
{
     initHanoi();
     saveScreen();
     splitScreen();
     vdBeginHanoi();
     mergeScreen();
}

/*-----------------------------------------------------------------------*/
int isColor(void) {
  union REGS r;
  r.h.ah = 15;
  return (int86(0x10, &r, &r) & 0x00ff) == 7 ? 0 : 1;
}

void setVRAM(void) {

  if(isColor())
    #ifdef INLINE_ASM
       VRAM = VRamColor;
    #else
       VRAM = (char far*)VRamColor;
    #endif
  else
    #ifdef INLINE_ASM
       VRAM = VRamMono;
    #else
       VRAM = (char far*)VRamMono;
    #endif
#if defined(INLINE_ASM)
  #if defined(DEBUG_VALUE)
   /* printf("%#lx\n", VRAM);*/
  #endif
#else
  #if defined(DEBUG_VALUE)
  /* printf("%#lx\n", VRAM);*/
  #endif
#endif
}

void pokeabyte(int x, int y, char abyte, int attr) {
#ifdef INLINE_ASM
  #ifdef COORD_ALIGN
    asm mov ax, y
    asm dec ax
    asm mov y , ax
    asm mov ax, x
    asm dec ax
  #else
    asm mov ax, x
  #endif
  asm shl ax, 1
  asm mov x , ax
  asm mov ax, VRAM
  asm mov es, ax
  asm mov ax, y
  asm mov bl, YShift
  asm mul bl
  asm mov bx, ax
  asm add bx, x
  asm mov dl, abyte
  asm mov dh, attr
  asm mov word ptr es:[bx], dx
#else
  char far *V = VRAM;
  #ifdef COORD_ALIGN
    V += (--y)*YShift + (--x)*XShift;
  #else
    V += y*YShift + x*XShift;
  #endif
  *V++ = abyte;
  *V = attr;
#endif
}

int peekabyte(int x, int y) {
int aword = 0x0000;
#ifdef INLINE_ASM
  #ifdef COORD_ALIGN
    asm mov ax, y
    asm dec ax
    asm mov y , ax
    asm mov ax, x
    asm dec ax
  #else
    asm mov ax, x
  #endif
  asm shl ax, 1
  asm mov x , ax
  asm mov al, y
  asm mov bl, YShift
  asm mul bl
  asm mov bx, ax
  asm add bx, x
  asm mov ax, VRAM
  asm mov es, ax
  asm mov dx, word ptr es:[bx]
  asm xchg dh, dl
  asm mov aword, dx
#else
  char far *V = VRAM;
  #ifdef COORD_ALIGN
    V += (--y)*YShift + (--x)*XShift;
  #else
    V += y*YShift + x*XShift;
  #endif
  aword = ((int)*V++)<<8;
  aword |= (int)*V;
#endif
 return aword;
}

void pokeastring(int x, int y, char *str, int attr) {

int i, len = strlen(str);
#ifdef INLINE_ASM
  for(i = 0; i < len; i++)
    pokeabyte(x+i, y, str[i], attr);
#else
#ifdef COORD_ALIGN
  --y; --x;
#endif
  char far *V;
  int ycon;
  ycon = y*YShift;
  for(i = 0; i < len; i++) {
    V = VRAM + ycon;
    V += (x+i)*XShift;
    *V++ = *str++;
    *V = attr;
  }
#endif
}

void drawHanoiTower(SPostID *p) {
     int i;

     p->yCurrentPost = 0;
     for ( i = diskNumber-1; i >= 0; i--) {
	   p->yCurrentPost++;
	   vdDrawDisk(i, p);
     }
}

void vdBeginHanoi(void)
{
     int  oldLengthOfPost = 0;
     char dnumber[2];

     acleartextHanoi(0,0,79,24,fillChar[2], SColor.erase);

     pokeastring(14, 0, "HANOI TOWER Ver 0.1 (C) 1997, Euisang Hwang", SColor.title);
     pokeastring(1 , 1, "F1:delay", SColor.message);
     printfHanoi(10, 1, LIGHTGREEN|Reverse, "%5d", delayTime);
     drawBottom();

     while(1) {
	  count = diskNumber = 0;

	  pokeastring(24,2,"If you want to quit press 'q'", SColor.message);
	  pokeastring(27,3,"1 <= disk number <= 13", SColor.message);
	  pokeastring(28,4,"Enter disk number:  ",SColor.message);
	  vdGetsHanoi(47, 4, dnumber, sizeof dnumber);
	  if( toupper(dnumber[0]) == 'Q') break;
	  diskNumber = atoi(dnumber);

	  if(diskNumber > MaximumDiskNumber || diskNumber < 1) {
		 putch(0x07);
		 pokeastring(20,7, "Error : disk number is too many or few", SColor.error|144);
		 pokeastring(32,8, "Press any key", SColor.error|144);
		 inkey();
		 acleartextHanoi(18, 3, 58, 8, fillChar[2], SColor.erase);
	  continue;
	  }
	  acleartextHanoi(24, 2, 52, 4, fillChar[2], SColor.erase);

	  postBeforeMoving.yCurrentPost = 0;
	  postBeforeMoving.postID = 0;

	  postBeingUsing.yCurrentPost = 0;
	  postBeingUsing.postID = 1;

	  postAfterMoving.yCurrentPost = 0;
	  postAfterMoving.postID = 2;

	  LengthOfPost = diskNumber+1;
	  acleartextHanoi(0, LowerLimit-oldLengthOfPost, 79, 24, fillChar[2], SColor.erase);
	  vdMakeFrame();
	  vdMakeDisk();
	  drawBottom();

	  drawHanoiTower(&postBeforeMoving);
	  vdMessageHanoi(28, 2, "Press any key to begin", SColor.message);
	  nputchHanoi(28,2, 22, fillChar[2], SColor.erase);
	  vdHanoiTower(diskNumber, &postBeforeMoving, &postBeingUsing, &postAfterMoving);
	  if(diskNumber > 13) drawHanoiTower(&postAfterMoving);

	  flushall();
	  vdDeleteDisk();
	  vdMessageHanoi(31, LowerLimit+2, "Press any key", SColor.message|128);
	  acleartextHanoi(10, LowerLimit+1, 60, LowerLimit+2, fillChar[2], SColor.erase);
	  /*nputchHanoi(31,LowerLimit+1, 14, fillChar[2], SColor.erase);*/
	  oldLengthOfPost = LengthOfPost;
     }
     pokeastring(24, 10, "...Thanks for using HANOI TOWER...", SColor.title);
     delay(1000);
     gotoxy(prevCursorX,prevCursorY);
}

/*//  BM : Post before moving
      BU : Post being using
      AM : Post ater moving //*/

void vdHanoiTower(int disknumber, SPostID *postBM, SPostID *postBU, SPostID *postAM)
{
     if ( disknumber > 0) {
	vdHanoiTower(disknumber-1, postBM, postAM , postBU);
	vdMoveDisk(disknumber-1, postBM, postAM);
	vdHanoiTower(disknumber-1, postBU , postBM, postAM);
     }
}

int  vdGetsHanoi(int x, int y, char *dnumber, int size)
{
     int i = 0;
     int temp;

     do {
	gotoxy(x+1+i, y+1);
	while(1) {
	  temp = inkey();
	  if( isdigit(temp) || temp != _ENTER ||
	      temp != _BS || temp != _ESC || toupper(temp) != 'Q') break;
	  else continue;
	}

	if (temp == _BS) dnumber[--i] = NULL;
	else if (temp == _ENTER) break;
	else if (temp == _ESC) break;
	else if (toupper(temp) == 'Q') {
		   dnumber[0] = temp;
		   i++;
		   break;
	     }
	else
	     dnumber[i] = temp, dnumber[++i] = NULL;

       if (i < 0) i = 0;
	nputchHanoi(x, y, size, fillChar[3], LIGHTGRAY);
	pokeastring(x, y, dnumber, LIGHTGREEN);
     } while(i <= size);

     dnumber[i] = NULL;
     nputchHanoi(x, y, size, fillChar[3], LIGHTGRAY);
     pokeastring(x, y, dnumber, LIGHTGREEN);

     return (temp);
}

void vdSetDelayTime(void)
{
     /*unsigned char *buffer;*/
     boxCharacter   msgBox = h2v2;
     char cDelaytime[10];
     static char message[] = "Enter delay time(miliseconds):";
     int  x1, y1, x2, y2, nLength0, nLength1; /*, tempDelay;*/
     int  prevX, prevY, key, inCoordX, inCoordY; /*, arraySize;*/

     noDoubleDelay = TRUE;
     prevX    = wherex();
     prevY    = wherey();
     nLength0 = sizeof(message);
     nLength1 = sizeof(cDelaytime);

     x1 = 3, y1 = 7;
     x2 = x1 + nLength0 + nLength1 +1;
     y2 = y1+2;
     inCoordX = x2 - nLength1 -1;
     inCoordY = y1+1;
     /*arraySize = (x2-x1+1)*(y2-y1+1)*2;*/
     /*
	buffer = new char [arraySize];
	(char *) calloc((x2-x1+1)*(y2-y1+1)*2, sizeof (char) );
	savetextHanoi( x1, y1, x2, y2, buffer);
     */
	boxHanoi(msgBox, x1, y1, x2, y2, LIGHTRED);
	pokeastring(x1+1, y1+1, message, SColor.message);
	nputchHanoi(inCoordX, inCoordY, nLength1+1, ' ', Reverse);
	key = vdGetsHanoi(inCoordX, inCoordY, cDelaytime, nLength1);
	acleartextHanoi(x1, y1, x2, y2, fillChar[2], SColor.erase);
	/*restoretextHanoi( x1, y1, x2, y2, buffer);*/

	if ( key != _ESC )  {
	     delayTime = atoi(cDelaytime);
	     printfHanoi(10, 1, LIGHTGREEN|Reverse, "%5d", delayTime);
	}
     gotoxy(prevX, prevY);
     noDoubleDelay = FALSE;
}

void vdMessageHanoi(int x, int y, char *message, int attrib)
{
     pokeastring(x, y, message, attrib);
     inkey();
     nputchHanoi(x, y, strlen(message), ' ', SColor.erase);
}

void vdMoveDisk(int nThDisk, SPostID *postFrom, SPostID *postTo)
{
     vdEraseDisk(nThDisk, postFrom);  /*/ erase Nth disk at postFrom   /*/
     postFrom->yCurrentPost--;        /*/ decrease y-coord of postFrom /*/
     postTo->yCurrentPost++;          /*/ increase y-coord of postTo   /*/
     vdDrawDisk(nThDisk, postTo);     /*/ draw Nth disk at postTo      /*/
     count++;
     printfHanoi(46, LowerLimit+1, SColor.diskcount|Reverse,"%6ld", count);
     delay(delayTime);
}

void vdDrawDisk(int nThDisk, SPostID *nThPost)
{
     int x, y;

     x = xCoordOfPost[nThPost->postID] - strlen(disk[nThDisk])/2;
     y = LowerLimit - nThPost->yCurrentPost;
     pokeastring(x, y, disk[nThDisk], SColor.disk);
}

void vdEraseDisk(int nThDisk, SPostID *nThPost)
{
     int i, x, y, disklen;

     disklen = strlen(disk[nThDisk]);
     x = xCoordOfPost[nThPost->postID] - disklen/2;
     y = LowerLimit - nThPost->yCurrentPost;

     for ( i = 0; i < disklen; i++)
	pokeabyte(x+i, y, fillChar[2], SColor.erase);

     x = xCoordOfPost[nThPost->postID];

     pokeabyte(x, y, VerticleLine, SColor.post);
}

void vdMakeDisk(void)
{
     int i, j, increase;

     disk = (unsigned char **) calloc(diskNumber, sizeof (char *));

     if(disk == NULL) {
	    puts("memory allocation error...");
	    exit(-1);
     }

     for ( i = increase = 1; i <= diskNumber; i++, increase += 2) {
	   disk[i-1] = (char *) calloc((increase+1), sizeof (char));

	   if(disk[i-1] == NULL) {
		   puts("memory allocation error...");
		   exit(-1);
	   }
	   for ( j = 0; j < increase; j++)
	      disk[i-1][j] = fillChar[3];
	   disk[i-1][j] = NULL;
     }
}

void vdDeleteDisk(void)
{
     int i;

     for ( i = 0; i < diskNumber; i++)
	   free(disk[i]);

     free(disk);
}

void vdMakeFrame(void)
{
     int i, j;

     for ( i = (LowerLimit-LengthOfPost); i <= LowerLimit; i++)  {
	   pokeabyte(xCoordOfPost[0], i, VerticleLine, SColor.post);
	   pokeabyte(xCoordOfPost[1], i, VerticleLine, SColor.post);
	   pokeabyte(xCoordOfPost[2], i, VerticleLine, SColor.post);
     }
     for ( j = 0; j < diskNumber; j++)
	   printfHanoi(0, (LowerLimit-diskNumber+j), SColor.disknumber,"%2d",j);

     nputchHanoi(0, LowerLimit, 80, HorizontleLine, SColor.post);
     pokeabyte(xCoordOfPost[0], LowerLimit, ReversedT, SColor.post);
     pokeabyte(xCoordOfPost[1], LowerLimit, ReversedT, SColor.post);
     pokeabyte(xCoordOfPost[2], LowerLimit, ReversedT, SColor.post);

     pokeastring(23, LowerLimit+1, "Number of disk moving : ", SColor.message);
}
/*/////////////////////////////////////////////////////////////////////////
//                                                                       //
// Video memory begins at 0xB000:0000 on monochrome and 0xB800:0000 on   //
// color. Specifically first byte is to put any character and attribute  //
// byte follows. If one wants to change the color of character, he shou- //
// ld put another value(color code:RED, GREEN and BLUE etc.) into the a- //
// ttribute byte. It consists of eight bits, each has its own meaning.   //
// Here are the bits structure and meanings.                             //
//                                                                       //
//    ---------------** Structure of Attribute Byte **---------------    //
//                                                                       //
//     7 6 5 4 3 2 1 0                                                   //
//             +-------------- Intensity                                 //
//     X       | +------------ Red    --+                                //
//     | X X X | | +---------- Green    |  Foreground color (character)  //
//     | | | | I | | +-------- Blue   --+                                //
//     | | | |   X X X                                                   //
//     | | | +---------------- Blue   --+                                //
//     | | +------------------ Green    |  Background color (attribute)  //
//     | +-------------------- Red    --+                                //
//     +---------------------- Blinking                                  //
//                                                                       //
/////////////////////////////////////////////////////////////////////////*/
void initHanoi(void)
{
   setVRAM();
   prevCursorX = wherex();
   prevCursorY = wherey();
   if(isColor()) {
      SColor.post       = LIGHTGREEN;
      SColor.title      = YELLOW | 16;
      SColor.message    = BROWN | Reverse;
      SColor.error      = LIGHTRED;
      SColor.disk       = LIGHTGRAY;
      SColor.disknumber = LIGHTMAGENTA;
      SColor.diskcount  = YELLOW;
      SColor.erase      = LIGHTBLUE;
   }
   else {
      SColor.post       = BrightNormal;
      SColor.title      = Reverse;
      SColor.message    = BrightNormal;
      SColor.error      = BlinkingNormal;
      SColor.disk       = Normal;
      SColor.disknumber = BrightNormal;
      SColor.diskcount  = BrightNormal;
      SColor.erase      = Normal;
   }
   clock_inter = in_null;
   setClockRoutine(watch);
   setXYCoordOfWatch(70,24,73,24,76,24);
   setColorOfWatch(LIGHTRED, LIGHTRED, LIGHTRED);
}

void nputchHanoi( int x, int y, int number, unsigned char munja, int attrib)
{
     int i;

     for ( i = 0; i < number; i++)
	  pokeabyte(x + i, y, munja, attrib);
}

void printfHanoi(int x, int y, int attrib, char *format,...)
{
     char cBuffer[80];
     va_list pArgumentPointer;

     va_start(pArgumentPointer, format);
     vsprintf(cBuffer, format, pArgumentPointer);
     pokeastring(x, y, cBuffer, attrib);
     va_end(pArgumentPointer);
}

void acleartextHanoi(int x1, int y1, int x2, int y2, unsigned char fill, int color)
{
   int i,j;

   for(j = y1; j <= y2; j++)
      for(i = x1; i <= x2; i++)
	  pokeabyte(i,j, fill, color);
}
/*-------------------------------------------------------------------*/
void in_null(void)
{ }

void setClockRoutine(void (*func)())
{
   clock_inter = func;
}

void clockRoutine(void)
{
   (*clock_inter)();
}

void  setColorOfWatch(int hour, int min, int sec)
{
      watchAttrib.hour = hour;
      watchAttrib.minute  = min;
      watchAttrib.second  = sec;
}

void  setXYCoordOfWatch(int hourX, int hourY, int minX, int minY,int secX, int secY)
{
      xyCoordination.secondX  =  secX;
      xyCoordination.secondY  =  secY;
      xyCoordination.minuteX  =  minX;
      xyCoordination.minuteY  =  minY;
      xyCoordination.hourX    =  hourX;
      xyCoordination.hourY    =  hourY;
}

void watch(void)
{
   static struct time prvTime;
   static struct time curTime;

   gettime(&curTime);

   if(curTime.ti_sec != prvTime.ti_sec)
   {
    printfHanoi(xyCoordination.secondX, xyCoordination.secondY, watchAttrib.second, "%2d", curTime.ti_sec);
       if(curTime.ti_min != prvTime.ti_min)
       {
	printfHanoi(xyCoordination.minuteX, xyCoordination.minuteY, watchAttrib.minute,"%2d", curTime.ti_min);
	   if(curTime.ti_hour != prvTime.ti_hour)
	   {
	      if(curTime.ti_hour % 12 == 0)  {
		 pokeastring(xyCoordination.hourX-3, xyCoordination.hourY, "AM", YELLOW);
		 printfHanoi(xyCoordination.hourX, xyCoordination.hourY, watchAttrib.hour,"12");
	      }
	      else {
		 if(curTime.ti_hour >= 12) {
		   pokeastring(xyCoordination.hourX-3, xyCoordination.hourY, "PM", YELLOW);
		   printfHanoi(xyCoordination.hourX, xyCoordination.hourY, watchAttrib.hour,"%2d", curTime.ti_hour%12);
		 }
		 else {
		   pokeastring(xyCoordination.hourX-3, xyCoordination.hourY, "AM", YELLOW);
		   printfHanoi(xyCoordination.hourX, xyCoordination.hourY, watchAttrib.hour,"%2d", curTime.ti_hour);
		 }
	      prvTime.ti_hour = curTime.ti_hour;
	      }
	    }
	 prvTime.ti_min = curTime.ti_min;
      }
      prvTime.ti_sec = curTime.ti_sec;
   }
}

void drawBottom(void)
{
   struct time curTime;

   gettime(&curTime);

    printfHanoi(xyCoordination.secondX, xyCoordination.secondY , watchAttrib.second, "%2d", curTime.ti_sec);
    printfHanoi(xyCoordination.minuteX, xyCoordination.minuteY, watchAttrib.minute,"%2d", curTime.ti_min);

    if(curTime.ti_hour % 12 == 0) {
       pokeastring(xyCoordination.hourX-3, xyCoordination.hourY, "AM", YELLOW);
       printfHanoi(xyCoordination.hourX, xyCoordination.hourY, watchAttrib.hour,"12");
    }
    else {
	  if(curTime.ti_hour >= 12) {
	     pokeastring(xyCoordination.hourX-3, xyCoordination.hourY, "PM", YELLOW);
	     printfHanoi(xyCoordination.hourX, xyCoordination.hourY, watchAttrib.hour,"%2d", curTime.ti_hour%12);
	  }
	  else {
	     pokeastring(xyCoordination.hourX-3, xyCoordination.hourY, "AM", YELLOW);
	     printfHanoi(xyCoordination.hourX, xyCoordination.hourY, watchAttrib.hour,"%2d", curTime.ti_hour);
	  }
    }
    pokeabyte(xyCoordination.minuteX-1,xyCoordination.minuteY, ':', LIGHTGREEN);
    pokeabyte(xyCoordination.secondX-1,xyCoordination.secondY, ':', LIGHTGREEN);
    pokeabyte(xyCoordination.hourX-1,xyCoordination.secondY, '|', LIGHTGREEN);
}

void boxHanoi(boxCharacter type, int x1, int y1, int x2, int y2, int attr)
{
   int i;

   for(i = (x1+1); i < x2; i++) {
      pokeabyte( i, y1, type.hrz, attr);
      pokeabyte( i, y2, type.hrz, attr);
   }
   for(i = (y1+1); i < y2; i++) {
      pokeabyte( x1, i, type.vrt, attr);
      pokeabyte( x2, i, type.vrt, attr);
   }
   pokeabyte( x1, y1, type.lft_top, attr);
   pokeabyte( x2, y1, type.rgt_top, attr);
   pokeabyte( x1, y2, type.lft_btm, attr);
   pokeabyte( x2, y2, type.rgt_btm, attr);
}

int inkey(void)
{
   int key;

   while(1) {
      while(!bioskey(1))clockRoutine();
	 if( (key = getkey()) == F1 && !noDoubleDelay) {
	      vdSetDelayTime();
	      continue;
	 }
	 else break;
   }
   return (key);
}

int getkey(void)
{
    unsigned  char high, low;
    int     key;

    key = bioskey(0);

    high = (unsigned) key >> 8;
    low = (unsigned) key;

    if(low)   return (low);
    else      return (high+256);
}

void savetextHanoi(int x1, int y1, int x2, int y2, unsigned char *buff)
{
   int i,j; unsigned int word;

   for(j = y1; j <= y2; j++)
      for(i = x1; i <= x2; i++) {
	 word = (unsigned int)peekabyte(x1, y1);
	 *buff++ = (word & 0xff00)>>8;
	 *buff++ = word & 0x00ff;
   }
}

void restoretextHanoi(int x1, int y1, int x2, int y2, unsigned char *buff)
{
   int i,j;

   for(j = y1; j <= y2; j++)
     for(i = x1; i <= x2; i++)
       pokeabyte(i, j, *buff++, *buff++);
   free(buff);
}

static char byteBuf[25][80], attrBuf[25][80];

void mergeScreen(void) {

  int xi, hwid = 40,hgt = 25;
  int yj, l, tmp;

  for(xi = hwid-1; xi >= 0; --xi) {
    for(yj = 0; yj < hgt; yj++) {
      for(l = 0; l < hwid-xi; ++l) {
	tmp = xi+l;
	pokeabyte(l, yj, byteBuf[yj][tmp], attrBuf[yj][tmp]);
	tmp = hwid+l;
	pokeabyte(tmp+xi, yj, byteBuf[yj][tmp], attrBuf[yj][tmp]);
      }
    }
    delay(10);
  }
}

void saveScreen(void) {

  int yj, xi;
  char far *V = VRAM;
  for(yj = 0; yj < 25; yj++) {
    V = VRAM + yj*YShift;
    for(xi = 0; xi < 80; xi++) {
      byteBuf[yj][xi] = *V++;
      attrBuf[yj][xi] = *V++;
    }
  }
}

void splitScreen(void) {

  int xi, hwid = 40, hgt = 25;
  int yj, l, tmp, sft;

  sft = 1;
  for(xi = 0; xi < hwid; xi++) {
    for(yj = 0; yj < hgt; yj++) {
      pokeabyte(hwid-xi-1, yj, fillChar[2], LIGHTBLUE);
      pokeabyte(hwid+xi, yj, fillChar[2], LIGHTBLUE);
      for(l = 0; l < hwid-xi-1; ++l) {
	tmp = l+sft;
	pokeabyte(l, yj, byteBuf[yj][tmp], attrBuf[yj][tmp]);
	tmp = hwid+l;
	pokeabyte(tmp+sft, yj, byteBuf[yj][tmp], attrBuf[yj][tmp]);
      }
    }
    ++sft;
    delay(10);
  }
}
/*/... End of HANOI.C ...  */
