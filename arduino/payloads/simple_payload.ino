#include <Keyboard.h>

void setup() {
  Keyboard.begin();
  delay(2000);

  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press(KEY_F2);
  Keyboard.releaseAll();

  delay(1000); 

  Keyboard.print("gedit");
  delay(100);

  Keyboard.write(KEY_RETURN); 

  delay(1000);

  Keyboard.print("This is a message for testing purposes to demonstrate how an Arduino can execute as a keyboard!");
  Keyboard.end();
}

void loop() {
}