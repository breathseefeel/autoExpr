#include "simuType.h"
#include <string>
#include <Windows.h>
#include <iostream>
using namespace std;
void simulateType(const char *str)
{
	int len = strlen(str);
	for(int i = 0; i < len; ++i){
		switch (str[i]){
		case ':':
			simulateMaohao();
			break;
		case ' ':
			keybd_event(VK_SPACE,0,0,0);
			keybd_event(VK_SPACE,0,KEYEVENTF_KEYUP,0);
			break;
		case '(':
			simulateLeftBrace();
			break;
		case ')':
			simulateRightBrace();
			break;
		case ',':
			keybd_event(VK_OEM_COMMA,0,0,0);
			keybd_event(VK_OEM_COMMA,0,KEYEVENTF_KEYUP,0);
			break;
		case '-':
			keybd_event(VK_OEM_MINUS,0,0,0);
			keybd_event(VK_OEM_MINUS,0,KEYEVENTF_KEYUP,0);
			break;
		case '_':
			keybd_event(VK_SHIFT,0,0,0);
			keybd_event(VK_OEM_MINUS,0,0,0);
			keybd_event(VK_OEM_MINUS,0,KEYEVENTF_KEYUP,0);
			keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
			break;
		default:
			if(str[i] >= 'a' && str[i] <= 'z'){
				keybd_event(str[i]-32,0,0,0);
				keybd_event(str[i]-32,0,KEYEVENTF_KEYUP,0);
			}
			else if(str[i] >= '0' && str[i] <= '9'){
				keybd_event(str[i]-'0' + 48,0,0,0);
				keybd_event(str[i]-'0' + 48,0,KEYEVENTF_KEYUP,0);
			}
			else if(str[i] == '\n'){
				keybd_event(VK_RETURN, 0, 0, 0); 
			}
			else{
				std::cout << "unknown character, press to exit!" << endl;
				getchar();
				exit(-1);
			}
		}
		

	}
}


void simulateTryNum(int i)
{
	keybd_event(i,0,0,0);
	keybd_event(i,0,KEYEVENTF_KEYUP,0);
}
void simulateNumber(int n)
{
	char buf[10];
	int i = 0, j = 0, len;
	if(n == 0){
		buf[i++] = '0';
	}
	while(n != 0){
		buf[i++] = '0' + n % 10;
		n /= 10;
	}
	len = i;
	buf[i--] = '\0';
	for(; j < i; j++, i--){
		buf[i] ^= buf[j];
		buf[j] ^= buf[i];
		buf[i] ^= buf[j];
	}
	for(i = 0; i < len; ++i){
		keybd_event(buf[i] - '0' + 48,0,0,0);
		keybd_event(buf[i] - '0' + 48,0,KEYEVENTF_KEYUP,0);
	}
}

void simulateLeftBrace(void)
{
	keybd_event(VK_SHIFT,0,0,0);
	keybd_event(48+9,0,0,0);
	keybd_event(48+9,0,KEYEVENTF_KEYUP,0);
	keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
}
void simulateRightBrace(void)
{
	keybd_event(VK_SHIFT,0,0,0);
	keybd_event(48,0,0,0);
	keybd_event(48,0,KEYEVENTF_KEYUP,0);
	keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
}
void simulateMaohao(void)
{
	keybd_event(VK_SHIFT,0,0,0);
	keybd_event(186,0,0,0);
	keybd_event(186,0,KEYEVENTF_KEYUP,0);
	keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
}
