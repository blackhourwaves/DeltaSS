#include "app.h"

#include <windows.h>

int main() {
    deltass::RunApplication(GetModuleHandleW(nullptr), SW_SHOW);
    return 0;
}