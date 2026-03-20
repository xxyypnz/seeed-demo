#include "Arduino.h"
#include <utility>

extern "C" void DebugLog(const char* s) {
    Serial.print(s);
}

namespace test_over_serial {

void SerialWrite(const char* buffer) {
    Serial.print(buffer);
}

std::pair<size_t, char*> SerialReadLine(int timeout) {
    return std::make_pair((size_t)0, (char*)nullptr);
}

}  // namespace test_over_serial
