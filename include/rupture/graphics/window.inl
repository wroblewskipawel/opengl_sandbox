#pragma once

#include <functional>

typedef std::function<void(void)> KeyPressFunction;
typedef std::function<void(void)> CursorPosFunction;
typedef std::function<void(int, int)> KeyCallback;
typedef std::function<void(int)> FocusCallback;
typedef std::function<void(void)> CloseCallback;
typedef std::function<void(double)> ScrollCallback;
typedef std::function<void(double, double)> CursorPosCallback;
typedef std::function<void(int, int)> ResizeCallback;

namespace gl {

class Window;

}