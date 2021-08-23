#pragma once

#include <Windows.h>
#include <DirectXMath.h>

class Input
{
#pragma region Singleton
public:
	// Gets the one and only instance of this class
	static Input& GetInstance()
	{
		if (!instance)
		{
			instance = new Input();
		}

		return *instance;
	}

	// Remove these functions (C++ 11 version)
	Input(Input const&) = delete;
	void operator=(Input const&) = delete;

private:
	static Input* instance;
	Input() {};
#pragma endregion

public:
	~Input();

	void Initialize(HWND windowHandle);
	void Update();
	void PostUpdate();

	bool GetKeyArray(bool* keyArray, int size = 256);

	int GetMouseX() { return mouseX; }
	int GetMouseY() { return mouseY; }
	int GetMouseXDelta() { return mouseXDelta; }
	int GetMouseYDelta() { return mouseYDelta; }
	float GetMouseWheel() { return wheelDelta; }

	bool KeyDown(int key);
	bool KeyUp(int key);

	bool KeyPress(int key);
	bool KeyRelease(int key);

	bool MouseLeftDown();
	bool MouseRightDown();
	bool MouseMiddleDown();

	bool MouseLeftUp();
	bool MouseRightUp();
	bool MouseMiddleUp();

	bool MouseLeftPress();
	bool MouseLeftRelease();

	bool MouseRightPress();
	bool MouseRightRelease();

	bool MouseMiddlePress();
	bool MouseMiddleRelease();

private:
	unsigned char* kbState;
	unsigned char* prevKbState;

	int mouseX;
	int mouseY;
	int prevMouseX;
	int prevMouseY;
	int mouseXDelta;
	int mouseYDelta;

	float wheelDelta;
	HWND windowHandle;
};

