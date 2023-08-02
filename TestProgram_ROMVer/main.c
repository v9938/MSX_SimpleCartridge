#include <stdio.h>
#include <msx.h>
#include <sys/ioctl.h>


//#include <msx/gfx.h>

unsigned char SelectSlot;		//Select Slot number
unsigned char transbuf[0x80];	//Page3 program temp area


// Jump Table
void do_eraseFlash() __z88dk_fastcall __naked  {
#asm
    jp _transbuf
#endasm
}

int do_chkSimpleROM() __z88dk_fastcall __naked {
#asm
    jp _transbuf+_chkSimpleROM - _eraseFlash
#endasm
}

// Page3 Transfer routine ////////////////////////////////////////////
void eraseFlash() __z88dk_fastcall __naked {
#asm
	di
	ld de,05555h			; CHIP ERASE ======================
	ld a,0aah				; Flash Address 1nd 0x5555 = 0xAA
    ld (de),a				; 
    
    ld a,(_SelectSlot)		; Slot Number
	ld e,055h				;
    ld hl,02aaah			; Flash Address 2nd 0x2AAA = 0x55
    call 0014h				; call WRSLT (Page0 usually use in MSXBIOS)

	ld de,05555h			; Flash Address 3rd 0x5555 = 0x80
	ld a,080h				;
    ld (de),a				;

	ld a,0aah				; Flash Address 4th 0x5555 = 0xaa
    ld (de),a				; 
    
    ld a,(_SelectSlot)	;Slot Number
	ld e,055h				; 
    ld hl,02aaah			; Flash Address 5th 0x2AAA = 0x55
    call 0014h				; call WRSLT

	ld de,05555h			; Flash Address 6th 0x5555 = 0x10
	ld a,010h				;
    ld (de),a				; do chip erase with busy
loop:
    jr loop					; Stack (It should be waiting erase busy)
#endasm
}
int chkSimpleROM() __z88dk_fastcall __naked {
#asm

	di
	ld de,05555h			; SOFT ID READ ======================
	ld a,0aah				;
    ld (de),a				; Flash Address 1st 0x5555 = 0xAA

    ld a,(_SelectSlot)		; Slot Number
	ld e,055h				;
    ld hl,02aaah			; Flash Address 2nd 0x2AAA = 0x55
    call 0014h				; call WRSLT (Page0 usually use in MSXBIOS)

	ld de,05555h
	ld a,090h				; Flash Address 3rd 0x5555 = 0x90
    ld (de),a				; 

	ld bc,(04000h)			; Read SOFT ID (Any Addresss)
	push bc					; 

	ld a,0aah				; Reset Command ====================
    ld (de),a				; Flash Address 1st 0x5555 = 0xaa

    ld a,(_SelectSlot)		; Slot Number
	ld e,055h				;
    ld hl,02aaah			; Flash Address 2nd 0x2AAA = 0x55
    call 0014h				; call WRSLT

	ld de,05555h			;
	ld a,0f0h				; Flash Address 3rd 0x5555 = 0xf0
    ld (de),a				; Chip Resert

	pop hl					; Return SOFT ID
	ei

	ret
#endasm
}
// End of Page3 Transfer ////////////////////////////////////////////

// Don't Move it ////////////////////////////////////////////////////
void copy_eraseFlash(){
	unsigned char *src;
	unsigned char *des;
	unsigned char size;
	
	// It should be coded using LDIR :-)
	src = (unsigned char *) eraseFlash;
	des = (unsigned char *) transbuf;
	size = copy_eraseFlash - eraseFlash;
	while (size != 0){
		*(des) = *(src);
		size--;
		src++;
		des++;
	}
}


int chget(){
#asm
	call 009fh				; CHGET
    ld l,a					; Return SLT
    ld h,0					
#endasm
}

void set_color(char forcolor, char bakcolor, char bdrcolor)
{
#asm
    ld   ix,0
    add  ix,sp
    ld   a,(ix+2)			; BDRCLR
    ld   (0f3ebh),a
    ld   a,(ix+4)			; BAKCLR
    ld   (0f3eah),a
    ld   a,(ix+6)			; FORCLR
    ld   (0f3e9h),a
	call 0062h				; CHGCLR
;   ld   ix,0111h			; CHGCLR
;   call 015fh				; EXTROM
#endasm
}
void set_text() __z88dk_fastcall __naked {
#asm
    call 00D2h				; TOTEXT
	call 00C3h
#endasm
}
void set_screen(unsigned char *p) __z88dk_fastcall __naked {
#asm
    ld a,(hl)
    jp 005fh				; CHGMOD 
#endasm
}

int get_myslot() __z88dk_fastcall __naked {
#asm
	push hl
	call 0138h				; RSLREG read primary slot #
	rrca					; move it to bit 0,1 of [Acc]
	rrca
	and	00000011b
	ld	c,a
	ld	b,0
	ld	hl,0FCC1H			; EXPTBL see if this slot is expanded or not
	add	hl,bc
	ld	c,a					; save primary slot #
	ld	a,(hl)				; get the slot is expanded or not
	and	80h
	or	c					; set MSB if so
	ld	c,a					; save it to [C]
	inc hl					; Point to SLTTBL entry
	inc	hl
	inc	hl
	inc	hl
	ld	a,(hl)				; Get what  is  currently  output
							; to expansion   slot    register
	and	00001100b
	or	c					; Finaly form slot address
	pop hl
    ld l,a					; Return SLT
    ld h,0			
	push hl
	ld	h,80h
	call	0024h			; ENASLT enable page 2
	pop hl
	ret
#endasm
}

	
	
void main()
{
	int mySlot;											// My Slot number
	int findId;											// Flash ID
	
	set_text();											// Screen Init & Setting 
	set_color(15,5,7);									// Set Color
	mySlot = get_myslot();								// Get My Slot number 
	if (mySlot < 0x80) mySlot = mySlot & 0x03;			// Slot Mask

	SelectSlot = (unsigned char) mySlot;				// Set Slot number for Asm work
	copy_eraseFlash();									// Program Transfer to Page3 RAM (Need 16K RAM)
	
	printf("64K Simple ROM SelfTest V1.1\n");			// Display Copyright
	printf("Copyright 2023/08/01 @v9938\n\n");
	printf("Currant Slot: %02x\n",SelectSlot);

	findId = do_chkSimpleROM();							// Search & Get Flash ID (jump page3 copy code)
	printf("Flash ROM ID: %04x \n              ",findId);
	
	if (findId == 0xb5bf) printf("SST39SF010A\n");		// Check Flash & Maker ID
	else if (findId == 0xb6bf) printf("SST39SF020A\n"); // Flash ID B5/B6/B7 = 010/020/040
	else if (findId == 0xb7bf) printf("SST39SF040A\n"); // Maker ID BD       = SST(Microchip)
	else {
		printf("UNKNOWN\n");							// Test Failed
		printf("\n!! TEST FAILED !!\n");
		printf("Please Reset/Power off.\n");
		while (1);
	}
	
	printf("\n== TEST PASSED ==");
	printf("\nHit [e] key Flash Erase!");

	while (chget()!='e');								// Wait input "e" key

	printf("\nFlash Erased...Done.\n");
	printf("Please Reset/Power off.\n");
	do_eraseFlash();									// Erase Flash (jump page3 copy code)
}
