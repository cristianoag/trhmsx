/*
	ps2protocol.c
	
	Handles the higher-level parts of the PS/2 protocol
	HID conversion, responding to host commands
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "ch559.h"
#include "usbhost.h"
#include "uart.h"
#include "ps2.h"
#include "data.h"
#include "ps2protocol.h"
#include "menu.h"
#include "mouse.h"
#include "pwm.h"
#include "keyboardled.h"
#include "xt.h"
#include "settings.h"
#include "system.h"
#include "scancode.h"

// repeatState -
// if positive, we're delaying - count up to repeatDelay then go negative
// if negative, we're repeating - count down to repeatRate then return to -1
// if zero, we ain't repeating
__xdata volatile int16_t RepeatState = 0;

__xdata volatile uint8_t RepeatKey = 0x00;
__xdata int16_t RepeatDelay = 7500;
__xdata int16_t RepeatRate = -1000;

__xdata char lastKeyboardHID[8];

uint8_t LEDDelayMs = 0;

// Mouse buffer needed for handling multi-byte commands and intellimouse detection 
__xdata uint8_t MouseBuffer[MOUSE_BUFFER_SIZE];

// runs in interrupt to keep timings
void RepeatTimer(void)
{
	if (RepeatState > 0)
		RepeatState++;
	else if (RepeatState < 0)
		RepeatState--;
}

__code int16_t DelayConv[] = {
	3750,
	7500,
	11250,
	15000};

__code int16_t RateConv[] = {
	-500,
	-562,
	-625,
	-688,
	-725,
	-811,
	-877,
	-938,
	-1000,
	-1128,
	-1250,
	-1376,
	-1500,
	-1630,
	-1744,
	-1875,
	-2000,
	-2239,
	-2500,
	-2727,
	-3000,
	-3261,
	-3488,
	-3750,
	-4054,
	-4545,
	-5000,
	-5556,
	-6000,
	-6522,
	-7143,
	-7500};

// Runs in main loop
void HandleRepeats(void)
{
	if (RepeatState > RepeatDelay)
	{
		SetRepeatState(-1);
	}
	else if (RepeatState < RepeatRate)
	{
		SendKeyboard( FlashSettings->KeyboardMode == MODE_PS2 ? HIDtoPS2_Make[RepeatKey] : HIDtoXT_Make[RepeatKey]);
		SetRepeatState(-1);
	}
}


#define SetKey(key,report) (report->KeyboardKeyMap[key >> 3] |= 1 << (key & 0x07))

#define GetOldKey(key,report) (report->oldKeyboardKeyMap[key >> 3] & (1 << (key & 0x07)))

__code uint8_t bitMasks[] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1F, 0x3F, 0x7F, 0xFF};
void processSeg(HID_SEG *currSeg, HID_REPORT *report, uint8_t *data)
{
	bool make = 0;
	uint8_t tmp = 0;
	uint16_t cnt, endbit;
	uint8_t *currByte;
	uint8_t pressed = 0;
	uint8_t ButSet = 0, ButReset = 0;
	int16_t tmpl;

	if (currSeg->InputType == MAP_TYPE_BITFIELD)
	{

		endbit = currSeg->startBit + currSeg->reportCount;
		tmp = currSeg->OutputControl;
		for (cnt = currSeg->startBit; cnt < endbit; cnt++)
		{

			pressed = 0;

			// find byte
			currByte = data + ((cnt) >> 3);

			// find bit
			if (*currByte & (0x01 << (cnt & 0x07)))
				pressed = 1;

			if (currSeg->OutputChannel == MAP_KEYBOARD)
			{
				if (pressed)
				{
					
					SetKey(tmp, report);
					if (!GetOldKey(tmp, report)) {
						report->keyboardUpdated = 1;
					}
				}
				else
				{
					
					if (GetOldKey(tmp, report)) {
						report->keyboardUpdated = 1;
					}
				}
			}
			else
			{
				switch (tmp)
				{
				case MAP_MOUSE_BUTTON1:
					MouseSet(0, pressed);
					break;
				case MAP_MOUSE_BUTTON2:
					MouseSet(1, pressed);
					break;
				case MAP_MOUSE_BUTTON3:
					MouseSet(2, pressed);
					break;
				case MAP_MOUSE_BUTTON4:
					MouseSet(3, pressed);
					break;
				case MAP_MOUSE_BUTTON5:
					MouseSet(4, pressed);
					break;
				}
			}

			tmp++;
		}
	}
	else
	{
		// find byte
		currByte = data + (currSeg->startBit >> 3);

		// find bits
		currSeg->value = ((*currByte) >> (currSeg->startBit & 0x07)) // shift bits so lsb of this seg is at bit zero
						 & bitMasks[currSeg->reportSize];			 // mask off the bits according to seg size

		if (currSeg->OutputChannel == MAP_KEYBOARD)
			report->keyboardUpdated = 1;

		if (currSeg->InputType == MAP_TYPE_THRESHOLD_ABOVE && currSeg->value > currSeg->InputParam)
		{
			make = 1;
		}
		else if (currSeg->InputType == MAP_TYPE_THRESHOLD_BELOW && currSeg->value < currSeg->InputParam)
		{
			make = 1;
		}
		else if (currSeg->InputType == MAP_TYPE_EQUAL && currSeg->value == currSeg->InputParam)
		{
			make = 1;
		}

		else
		{
			make = 0;
		}
		// hack for mouse, as it needs to explicity switch on and off
		// this needs rewritten
		if (currSeg->OutputChannel == MAP_MOUSE && currSeg->InputType == MAP_TYPE_THRESHOLD_ABOVE) {
			switch (currSeg->OutputControl)
				{
				case MAP_MOUSE_BUTTON1:
					MouseSet(0, currSeg->value);
					break;
				case MAP_MOUSE_BUTTON2:
					MouseSet(1, currSeg->value);
					break;
				case MAP_MOUSE_BUTTON3:
					MouseSet(2, currSeg->value);
					break;
				case MAP_MOUSE_BUTTON4:
					MouseSet(3, currSeg->value);
					break;
				case MAP_MOUSE_BUTTON5:
					MouseSet(4, currSeg->value);
					break;
				}
		}
		else if (make)
		{


			if (currSeg->OutputChannel == MAP_KEYBOARD)
			{
				SetKey(currSeg->OutputControl, report);
			}
		}
		else if (currSeg->InputType == MAP_TYPE_SCALE)
		{
			if (currSeg->OutputChannel == MAP_MOUSE)
			{

				#define DEADZONE 1

				switch (currSeg->OutputControl)
				{
				// TODO scaling
				case MAP_MOUSE_X:
					if (currSeg->InputParam == 2){

						tmpl = ((int8_t)((currSeg->value + 8) >> 4) - 0x08);

						// deadzone
						if (tmpl <= -DEADZONE) tmpl+= DEADZONE;
						else if (tmpl >= DEADZONE) tmpl-= DEADZONE;
						else tmpl = 0;
						
						MouseMove(tmpl, 0, 0);
					}
					else

					MouseMove((int8_t)currSeg->value, 0, 0);

					break;
				case MAP_MOUSE_Y:
					if (currSeg->InputParam == 2){

						tmpl = ((int8_t)((currSeg->value + 8) >> 4) - 0x08);

						// deadzone
						if (tmpl <= -DEADZONE) tmpl+= DEADZONE;
						else if (tmpl >= DEADZONE) tmpl-= DEADZONE;
						else tmpl = 0;

						MouseMove(0, tmpl, 0);
					}
					else
					

					MouseMove(0, (int8_t)currSeg->value, 0);

					break;
				case MAP_MOUSE_WHEEL:
					/*if (currSeg->InputParam == 2)
						currSeg->value = (uint8_t)(-((int8_t)((currSeg->value + 8) >> 4) - 0x08));
					else*/

					MouseMove(0, 0, (int8_t)currSeg->value);

					break;
				}
			}
		}
		else if (currSeg->InputType == MAP_TYPE_ARRAY)
		{
			if (currSeg->OutputChannel == MAP_KEYBOARD)
			{
				SetKey(currSeg->value, report);
			}
		}
	}
}

bool BitPresent(uint8_t *bitmap, uint8_t bit)
{
	if (bitmap[bit >> 3] & (1 << (bit & 0x07)))
		return 1;
	else
		return 0;
}
 
bool ParseReport(INTERFACE *interface, uint32_t len, uint8_t *report)
{
	static HID_REPORT *descReport;
	static LinkedList *currSegNode;
	static HID_SEG *currSegment;

	// Turn off LEDs for a while
#if defined(BOARD_MICRO)
	SetPWM2Dat(0x00);
#elif defined(BOARD_PS2)
	P0 |= 0b01110000;
#else
	SetPWM1Dat(0x00);
	SetPWM2Dat(0x00);
	T3_FIFO_L = 0;
	T3_FIFO_H = 0;
#endif
	LEDDelayMs = 33;



	if (interface->usesReports)
	{
		// first byte of report will be the report number
		//descReport = interface->reports[report[0]];
		descReport = (HID_REPORT *)ListGetData(interface->Reports, report[0]);
	}
	else
	{
		descReport = (HID_REPORT *)ListGetData(interface->Reports, 0);
	}
	if (descReport == NULL){
		DEBUGOUT("Invalid report\n");
		return 0;
	}

	// sanity check length - smaller is no good
	if (len < descReport->length)
	{
		DEBUGOUT("report too short - %lu < %u\n", len, descReport->length);
		return 0;
	}

	currSegNode = descReport->HidSegments;

	// clear key map as all pressed keys should be present in report
	memset(descReport->KeyboardKeyMap, 0, 32);

	// TODO handle segs that are bigger than 8 bits
	while (currSegNode != NULL)
	{
		processSeg((HID_SEG *)(currSegNode->data), descReport, report);
		currSegNode = currSegNode->next;
	}

	if (descReport->keyboardUpdated)
	{
		for (uint8_t c = 0; c < 255; c++)
		{
			if (BitPresent(descReport->KeyboardKeyMap, c) && !BitPresent(descReport->oldKeyboardKeyMap, c))
			{
				if (MenuActive)
					Menu_Press_Key(c);
				else
				{
					//DEBUGOUT("\nSendn %x\n", c);
					// Make
					if (c <= 0x67)
					{
						SendKeyboard(FlashSettings->KeyboardMode == MODE_PS2 ? HIDtoPS2_Make[c] : HIDtoXT_Make[c]);
						RepeatKey = c;
						SetRepeatState(1);
					}
					else if (c >= 0xE0 && c <= 0xE7)
					{
						SendKeyboard(FlashSettings->KeyboardMode == MODE_PS2 ? ModtoPS2_MAKE[c - 0xE0] : ModtoXT_MAKE[c - 0xE0]);
					}
				}
			}
			else if (!BitPresent(descReport->KeyboardKeyMap, c) && BitPresent(descReport->oldKeyboardKeyMap, c))
			{
				if (!MenuActive)
				{
					// break
					if (c <= 0x67)
					{
						//DEBUGOUT("\nBreakn %x\n", c);
						// if the key we just released is the one that's repeating then stop
						if (c == RepeatKey)
						{
							RepeatKey = 0;
							SetRepeatState(0);
						}

						// Pause has no break for some reason
						if (c == 0x48)
							continue;

						SendKeyboard(FlashSettings->KeyboardMode == MODE_PS2 ? HIDtoPS2_Break[c] : HIDtoXT_Break[c]);
					}
					else if (c >= 0xE0 && c <= 0xE7)
					{
						SendKeyboard(FlashSettings->KeyboardMode == MODE_PS2 ? ModtoPS2_BREAK[c - 0xE0] : ModtoXT_BREAK[c - 0xE0]);
					}
				}
			}
			
		}

		memcpy(descReport->oldKeyboardKeyMap, descReport->KeyboardKeyMap, 32);
		descReport->keyboardUpdated = 0;
	}

	return 1;
}

void TypematicDefaults(void)
{
	RepeatDelay = DelayConv[0x01]; // 500ms
	RepeatRate = RateConv[0x0B];   // 10.9cps
}

void HandleReceived(uint8_t port)
{
	if (port == PORT_KEY)
	{
		// see if this is a command byte - these should override any state we're already in so check these first
		switch (ports[PORT_KEY].recvout)
		{

			// set LEDs
			case 0xED:
				SimonSaysSendKeyboard(KEY_ACK);
				ports[PORT_KEY].recvstate = R_LEDS;
				break;

			// Echo
			case 0xEE:
				SimonSaysSendKeyboard(KEY_ECHO);
				ports[PORT_KEY].recvstate = R_IDLE;
				break;

			// Set/Get scancode set
			case 0xF0:
				SimonSaysSendKeyboard(KEY_ACK);
				ports[PORT_KEY].recvstate = R_SCANCODESET;
				break;

			// ID
			case 0xF2:
				SimonSaysSendKeyboard(KEY_ACK);
				SimonSaysSendKeyboard(KEY_ID);
				ports[PORT_KEY].recvstate = R_IDLE;
				break;

			// set repeat
			case 0xF3:
				SimonSaysSendKeyboard(KEY_ACK);
				ports[PORT_KEY].recvstate = R_REPEAT;
				break;

			// Enable (TODO : actually do this)
			case 0xF4:
				SimonSaysSendKeyboard(KEY_ACK);
				ports[PORT_KEY].recvstate = R_IDLE;
				break;

			// Disable (TODO : actually do this)
			case 0xF5:
				SimonSaysSendKeyboard(KEY_ACK);
				TypematicDefaults();
				ports[PORT_KEY].recvstate = R_IDLE;
				break;

			// Set Default
			case 0xF6:
				SimonSaysSendKeyboard(KEY_ACK);
				TypematicDefaults();
				ports[PORT_KEY].recvstate = R_IDLE;
				break;

			// Set All Keys whatever (TODO : Actually do this)
			case 0xF7:
			case 0xF8:
			case 0xF9:
			case 0xFA:
				SimonSaysSendKeyboard(KEY_ACK);
				ports[PORT_KEY].recvstate = R_IDLE;
				break;

			// Set specific Keys whatever (TODO : Actually do this)
			case 0xFB:
			case 0xFC:
			case 0xFD:
				SimonSaysSendKeyboard(KEY_ACK);
				ports[PORT_KEY].recvstate = R_KEYLIST;
				break;

			// resend - TODO figure out how to deal with this
			case 0xFE:
				SimonSaysSendKeyboard(KEY_ACK);
				break;

			// reset
			case 0xFF:
				SimonSaysSendKeyboard(KEY_ACK);
				TypematicDefaults();
				SimonSaysSendKeyboard(KEY_BATCOMPLETE);
				ports[PORT_KEY].recvstate = R_IDLE;
				break;

			// not a command byte - this is fine if we're in another sstate, otherwise we send an error
			default:
				switch (ports[PORT_KEY].recvstate)
				{

					case R_LEDS:
						SimonSaysSendKeyboard(KEY_ACK);
						SetKeyboardLedStatusFromPS2(ports[PORT_KEY].recvout);
						ports[PORT_KEY].recvstate = R_IDLE;
						break;

					case R_REPEAT:
						SimonSaysSendKeyboard(KEY_ACK);
						// Bottom 5 bits are repeate rate
						RepeatRate = RateConv[ports[PORT_KEY].recvout & 0x1F];

						// top 3 bits are delay time
						RepeatDelay = DelayConv[(ports[PORT_KEY].recvout >> 5) & 0x03];

						ports[PORT_KEY].recvstate = R_IDLE;
						break;

					case R_SCANCODESET:
						SimonSaysSendKeyboard(KEY_ACK);

						// if request is zero then PC is expecting us to send current scan code set number#
						// TODO : allow changing scancode set and reporting correctly
						if (ports[PORT_KEY].recvout == 0)
							SimonSaysSendKeyboard(KEY_SCANCODE_2);

						ports[PORT_KEY].recvstate = R_IDLE;
						break;

					case R_KEYLIST:
						// Keep sending ACKs until we get a valid command scancode (which will be dealt with by the previous section)
						// TODO : actually log these keys and apply their settings appropriately
						// (not that I've ever seen any programs actually require this)
						SimonSaysSendKeyboard(KEY_ACK);
						break;

					case R_IDLE:
					default:
						// this means we receieved a non-command byte when we are expecting one, send error
						SimonSaysSendKeyboard(KEY_ERROR);
						ports[PORT_KEY].recvstate = R_IDLE;
						break;
				}

				break;
		}

		// If we're not expecting more stuff,
		// unlock the send buffer so main loop can send stuff again
		if (ports[PORT_KEY].recvstate == R_IDLE)
		{
			ports[PORT_KEY].sendDisabled = 0;
		}
	}

	else if (port == PORT_MOUSE)
	{
		switch (ports[PORT_KEY].recvstate)
		{
		case R_IDLE:
		
			// push command to buffer
			memmove(MouseBuffer+1, MouseBuffer, MOUSE_BUFFER_SIZE-1);
			MouseBuffer[0] = ports[port].recvout;

			switch (ports[port].recvout)
			{
				
			// Set Scaling 1:1
			case 0xE6:
				SimonSaysSendMouse1(0xFA); // ACK
				Ps2MouseSetScaling(MOUSE_PS2_SCALING_1X);
				break;
				
			// Set Scaling 2:1
			case 0xE7: 
				SimonSaysSendMouse1(0xFA); // ACK
				Ps2MouseSetScaling(MOUSE_PS2_SCALING_2X);
				break;
				
			// Set Resolution (need 1 additional data byte)
			case 0xE8: 
				SimonSaysSendMouse1(0xFA); // ACK
				break;
			
			// Status Request
			case 0xE9:							
				// TODO: construct bytes from real state
				SimonSaysSendMouse1(0xFA);		// ACK
				SimonSaysSendMouse3(0b00100000, // Stream Mode, Scaling 1:1, Enabled, No buttons pressed
									0x02,		// Resolution 4 counts/mm
									100);		// Sample rate 100/sec
				break;

			// ID
			case 0xF2:
				SimonSaysSendMouse1(0xFA); // ACK
				SimonSaysSendMouse1((&OutputMice[MOUSE_PORT_PS2])->Ps2Type); // Mouse type
				break;

			// Enable Reporting
			case 0xF4: 
				SimonSaysSendMouse1(0xFA); // ACK
				Ps2MouseSetReporting(MOUSE_PS2_REPORTING_ON);
				break; 
				
			// Disable Reporting
			case 0xF5: 
				SimonSaysSendMouse1(0xFA); // ACK
				Ps2MouseSetReporting(MOUSE_PS2_REPORTING_OFF);
				break;
			
			// Set Defaults
			case 0xF6: 
				SimonSaysSendMouse1(0xFA); // ACK
				Ps2MouseSetDefaults();
				break;
			
			// Reset
			case 0xFF:
				SimonSaysSendMouse1(0xFA); // ACK
				SimonSaysSendMouse1(0xAA); // POST OK
				SimonSaysSendMouse1(0x00); // Squeek Squeek I'm a mouse
				Ps2MouseSetType(MOUSE_PS2_TYPE_STANDARD);
				Ps2MouseSetDefaults();
				break;

			// unimplemented command
			case 0xEA: // Set Stream Mode
			case 0xEB: // Read Data
			case 0xEC: // Reset Wrap Mode
			case 0xEE: // Set Wrap Mode
			case 0xF0: // Remote Mode
			case 0xF3: // Set Sample Rate
			case 0xFE: // Resend
			default:   // argument from command?
				if (MouseBuffer[1] == 0xE8) 
				{
					// previous command was set Resolution, this should be actual resolution
					SimonSaysSendMouse1(0xFA); // ACK
					Ps2MouseSetResolution(MouseBuffer[0]);
				} 
				else
				{
					SimonSaysSendMouse1(0xFA); // Just smile and nod
				}
				break;
			}
		}
		
		if (HMSettings.Intellimouse){
			// enable intellimouse support if driver tried to to detect it
			if (memcmp(MouseBuffer, "\x50\xF3\x64\xF3\xC8\xF3", 6) == 0)
			{
				Ps2MouseSetType(MOUSE_PS2_TYPE_INTELLIMOUSE_3_BUTTON);
			}
			else if (memcmp(MouseBuffer, "\x50\xF3\xC8\xF3\xC8\xF3", 6) == 0)
			{
				Ps2MouseSetType(MOUSE_PS2_TYPE_INTELLIMOUSE_5_BUTTON);
			}
		}
		
		// If we're not expecting more stuff,
		// unlock the send buffer so main loop can send stuff again
		if (ports[PORT_MOUSE].recvstate == R_IDLE)
		{
			ports[PORT_MOUSE].sendDisabled = 0;
		}
	}
}