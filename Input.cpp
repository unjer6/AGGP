#include "Input.h"

// Singleton requirement
Input* Input::instance;

Input::~Input()
{
	delete[] kbState;
	delete[] prevKbState;
}

void Input::Initialize(HWND windowHandle)
{
	kbState = new unsigned char[256];
	prevKbState = new unsigned char[256];

	memset(kbState, 0, sizeof(unsigned char) * 256);
	memset(prevKbState, 0, sizeof(unsigned char) * 256);

	wheelDelta = 0.0f;
	mouseX = 0; mouseY = 0;
	prevMouseX = 0; prevMouseY = 0;
	mouseXDelta = 0; mouseYDelta = 0;

	this->windowHandle = windowHandle;
}

void Input::Update()
{
	// Copy the old keys
	memcpy(prevKbState, kbState, sizeof(unsigned char) * 256);

	// Get the latest keys
	GetKeyboardState(kbState);

	// Get the current mouse position, relative to the window
	POINT mousePos = {};
	GetCursorPos(&mousePos);
	ScreenToClient(windowHandle, &mousePos);

	// Save the individual components
	mouseX = mousePos.x;
	mouseY = mousePos.y;
}

void Input::PostUpdate()
{
	// Calculate the mouse delta
	mouseXDelta = mouseX - prevMouseX;
	mouseYDelta = mouseY - prevMouseY;

	// Save the position for next frame
	prevMouseX = mouseX;
	prevMouseY = mouseY;
}


bool Input::GetKeyArray(bool* keyArray, int size)
{
	if (size <= 0 || size > 256) return false;

	for (int i = 0; i < size; i++)
		keyArray[i] = !!(kbState[i] & 0x80);

	return true;
}

bool Input::KeyDown(int key)
{
	if (key < 0 || key > 255) return false;

	return (kbState[key] & 0x80) != 0;
}

bool Input::KeyUp(int key)
{
	if (key < 0 || key > 255) return false;

	return !(kbState[key] & 0x80);
}

bool Input::KeyPress(int key)
{
	if (key < 0 || key > 255) return false;

	return
		kbState[key] & 0x80 &&			// Down now
		!(prevKbState[key] & 0x80);		// Up last frame
}

bool Input::KeyRelease(int key)
{
	if (key < 0 || key > 255) return false;

	return
		!(kbState[key] & 0x80) &&	// Up now
		prevKbState[key] & 0x80;	// Down last frame
}

bool Input::MouseLeftDown() { return (kbState[VK_LBUTTON] & 0x80) != 0; }
bool Input::MouseRightDown() { return (kbState[VK_RBUTTON] & 0x80) != 0; }
bool Input::MouseMiddleDown() { return (kbState[VK_MBUTTON] & 0x80) != 0; }

bool Input::MouseLeftUp() { return !(kbState[VK_LBUTTON] & 0x80); }
bool Input::MouseRightUp() { return !(kbState[VK_RBUTTON] & 0x80); }
bool Input::MouseMiddleUp() { return !(kbState[VK_MBUTTON] & 0x80); }

bool Input::MouseLeftPress() { return kbState[VK_LBUTTON] & 0x80 && !(prevKbState[VK_LBUTTON] & 0x80); }
bool Input::MouseLeftRelease() { return !(kbState[VK_LBUTTON] & 0x80) && prevKbState[VK_LBUTTON] & 0x80; }

bool Input::MouseRightPress() { return kbState[VK_RBUTTON] & 0x80 && !(prevKbState[VK_RBUTTON] & 0x80); }
bool Input::MouseRightRelease() { return !(kbState[VK_RBUTTON] & 0x80) && prevKbState[VK_RBUTTON] & 0x80; }

bool Input::MouseMiddlePress() { return kbState[VK_MBUTTON] & 0x80 && !(prevKbState[VK_MBUTTON] & 0x80); }
bool Input::MouseMiddleRelease() { return !(kbState[VK_MBUTTON] & 0x80) && prevKbState[VK_MBUTTON] & 0x80; }
