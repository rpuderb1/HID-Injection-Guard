#include <Keyboard.h>

void setup() {
  Keyboard.begin();
  delay(2000);

  Keyboard.press(KEY_LEFT_CTRL);
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press('t');
  Keyboard.releaseAll();

  delay(1500);

  Keyboard.print("htop");
  Keyboard.write(KEY_RETURN);
}


void loop() {
}
