/*
	SimpleROM Flash ROM programmer
	Copyright 2021 @V9938
	
	V1.0		1st version
*/

#include <stdio.h>
#include<string.h>
#include <msx.h>
#include <sys/ioctl.h>

#define VERSION "V1.0"
#define DATE "2021/07"

#define BUF_SIZE 128

//Global
unsigned char SelectSlot;		//Select Slot number
unsigned char *addressWrite;	//Write pointer
unsigned char eseSlot;			//SimpleROM Slot number
unsigned char maxBank;			//ROM Size
int findId;						//Flash ID


//Assembler SubRoutine
void asmCalls(){
#asm
serupCmd:
	di
    ld a,(_SelectSlot)	;Slot Number
	ld e,0aah			;
    ld hl,05555h		;Flash Address 2nd 0x5555
    call 0014h			;call WRSLT

    ld a,(_SelectSlot)	;Slot Number
	ld e,055h			;
    ld hl,02aaah		;Flash Address 1st 0x2AAA
    jp 0014h			;call WRSLT

#endasm
}
void eraseSimpleROM() __z88dk_fastcall __naked {
#asm
	; Chip-Erase Command 1st
    ; aa-55-80

	call	serupCmd
    ld a,(_SelectSlot)	;Slot Number
	ld e,080h			;
    ld hl,05555h		;Flash Address 2nd 0x5555
    call 0014h			;call WRSLT

	; Chip-Erase Command 2nd
	; aa-55-10
	call	serupCmd
    ld a,(_SelectSlot)	;Slot Number
	ld e,010h			;
    ld hl,05555h		;Flash Address 2nd 0x5555
    call 0014h			;call WRSLT
	ei

    ld hl,0fc9eh		;JIFFY address
	ld a,(hl)			;Load INTCNT
	add 06h				;16.6ms x 6 = 99.6ms
busyWait:
	cp	(hl)			;Check INTCNT
	jr nz,busyWait		;Wait Loop
	ret
#endasm
}

void writeSimpleROM(unsigned char *Address,unsigned char data)
{
#asm
	ld ix,2
	add ix, sp
	ld h, (ix+3)		;address_h
	ld l, (ix+2)		;address_L
	ld e, (ix)			;data
	push de
	push hl
;
; Flash Byte-Program
; aa-55-a0

	call	serupCmd
    ld a,(_SelectSlot)	;Slot Number
	ld e,0a0h			;
    ld hl,05555h		;Flash Address 2nd 0x5555
    call 0014h			;call WRSLT

    ld a,(_SelectSlot)	;Slot Number
	pop hl
	pop de
	push de
	push hl
    call 0014h			;call WRSLT
	pop hl
	pop de
writeWait:	
	push de
	push hl
    ld a,(_SelectSlot)	;Slot Number
    call 000ch			;call RDSLT
	pop hl
	pop de
    cp e					; Verify check
    jr nz,writeWait			; Wait Byte-Program Time
	ei
	ret
#endasm
}

int chkSimpleROM() __z88dk_fastcall __naked {
#asm
	call	serupCmd

    ld a,(_SelectSlot)	;Slot Number
	ld e,090h			;
    ld hl,05555h		;Flash Address 2nd 0x5555
    call 0014h			;call WRSLT

    ld a,(_SelectSlot)	;Slot Number
    ld hl,04000h		;Read Maker ID
    call 000ch			;call RDSLT
    ld b,a
	push bc
    ld a,(_SelectSlot)	;Slot Number
    ld hl,04001h		;Read Read Device ID
    call 000ch			;call RDSLT
	pop bc
    ld c,a
    push bc

	; Reset Command sequence aa-55-f0
	call	serupCmd

    ld a,(_SelectSlot)	;Slot Number
	ld e,0f0h			;
    ld hl,05555h		;Flash Address 2nd 0x5555
    call 0014h			;call WRSLT
	pop hl
	ei
	ret
#endasm
}

void findSimpleROM(void){
	unsigned char i;
	unsigned int chipId;
	
	if ((SelectSlot & 0xf0) == 0x80){
		//Expantion Slot Check
		for (i=0;i<4;i++){
			chipId = chkSimpleROM();
			if ((chipId & 0xff00) == 0xbf00) {		//0xBF SST Maker ID
				eseSlot = SelectSlot;
				findId = chipId;
			}
			SelectSlot = SelectSlot + 4;
		}
	}else{
		//Master Slot Check
		chipId = chkSimpleROM();
		if ((chipId & 0xff00) == 0xbf00) {			//0xBF SST Maker ID
			eseSlot = SelectSlot;
			findId = chipId;
		}
	}
}
	
int main(int argc,char *argv[])
{

	FILE *fp;
	unsigned char ReadData[BUF_SIZE];
	unsigned int  ReadPt,WriteMax;
	unsigned char WriteData;
	int i;
	
	printf("Simple Cartridge Writer %s\n",VERSION);
	printf("Copyrigth %s @v9938\n\n",DATE);

	if (argc<2){
		printf( "simplew [rom files]\n");
		printf( "This program will write the selected file to the flash ROM.\n");
		printf( "Option :\n");
		printf( " /0 : Start address 0x0000\n");
		printf( " /4 : Start address 0x4000 (Defaults)\n");
		printf( " /8 : Start address 0x8000\n");
		return 0;
	}

	addressWrite = (unsigned char *)0x4000;
    for(i=1; i<argc; i++) {
		if (strncmp(argv[i],"/0",2) == 0) 	addressWrite = (unsigned char *)0x0000;
		if (strncmp(argv[i],"/4",2) == 0) 	addressWrite = (unsigned char *)0x4000;
		if (strncmp(argv[i],"/8",2) == 0) 	addressWrite = (unsigned char *)0x8000;
    }
    for(i=1; i<argc; i++) {
		if (strncmp(argv[i],"/",1) != 0) break;
    }
	

	printf("Search Flash ... ");
	eseSlot = 0;
	//Slot 1 Search
	SelectSlot = *((unsigned char *)0xfcc2) | 0x01;		//EXPTBL (SLOT1)
	findSimpleROM();
	//Slot 2 Search
	SelectSlot = *((unsigned char *)0xfcc3) | 0x02;		//EXPTBL (SLOT2)
	findSimpleROM();
	//Slot 3 Search
	SelectSlot = *((unsigned char *)0xfcc4) | 0x03;		//EXPTBL (SLOT3)
	findSimpleROM();
//SelectSlot = 0x01;
//eseSlot = 0x01;
//findId = 0xb5;
	if (eseSlot == 0) {								// Not find SimpleROM
		printf("NOT find\n");
		printf("Bye...\n");
		return -1;
	}else{
		printf("Find!\n");
		printf("\nSlot: %02x",eseSlot);
	}

	//Flash Type Check
	printf("\nFlash Type: ");
	if ((findId & 0x00ff) == 0xb5){
		printf("SST39SF010A\n");
		maxBank = 0xf;
	}else if ((findId & 0x00ff) == 0xb6){
		printf("SST39SF020A\n");
		maxBank = 0x1f;
	}else if ((findId & 0x00ff) == 0xb7){
		printf("SST39SF040A\n");
		maxBank = 0x3f;
	}else{
		printf("Unknown Flash\n");
		printf("Bye...\n");
		return -1;
	}

	//ROM File Open
	fp =fopen(argv[i],"rb");
    if( fp == NULL ){
    	printf( "File can't open... %s\n", argv[1]);
    	return -1;
    }

	//Bank set
	SelectSlot = eseSlot;

	//Flash Erase
	printf("\nFlash Erase ... ");
	eraseSimpleROM();
	printf("Done.");

	//Flash Write
	printf("\nStart address : 0x%04x",addressWrite);
	printf("\n");
	
	while (1){

		fread(ReadData,sizeof ReadData, 1, fp);		//Read Buffer

		if (feof(fp) != NULL) break;				//File End ?
		if (ferror(fp) != NULL) {					//EIO Error?
			printf("File IO Error!\n");
			fclose(fp);
			return -1;
		}

		//Parameter Check
		if (addressWrite >= 0xC000){				// 4000-Bfffh Write finsh?
			printf("\nROM size is FULL.");			// Ummm.ROM is FULL
			break;									//
		}else{
//			printf("\x1e\x0dFlash Write ... 0x%04x\n",addressWrite);
			printf("\x0dFlash Write ... 0x%04x-%04x",addressWrite,addressWrite+BUF_SIZE-1);
			for (ReadPt = 0;ReadPt<BUF_SIZE;ReadPt++){
				WriteData = *(ReadData+ReadPt);
				writeSimpleROM(addressWrite,WriteData);	//Write Data
				addressWrite ++;
			}
		}
	}
	fclose(fp);
	printf("\n\nDone. Thank you using!\n");
	return 0;

}
