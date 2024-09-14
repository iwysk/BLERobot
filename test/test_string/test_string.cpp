#include <Arduino.h>

void setup(void) {
    Serial.begin(115200);
    String string1 = "abcdefghijk";
    string1.remove(5);
    Serial.println(string1);
    String string2 = "abcdefghijk";
    string2.remove(5, 1);
    Serial.println(string2);

}
void loop(void) {

}