#include <Bluepad32.h>
#include "Adafruit_MCP23X17.h"

// defines
#define clawServoPin 5
#define auxServoPin 18
#define cabLights 32
#define auxLights 33

#define pivot0 15
#define pivot1 14
#define mainBoom0 9
#define mainBoom1 8
#define secondBoom0 0
#define secondBoom1 1
#define tiltAttach0 3
#define tiltAttach1 2
#define thumb0 11
#define thumb1 10
#define auxAttach0 12
#define auxAttach1 13

#define leftMotor0 7
#define leftMotor1 6
#define rightMotor0 4
#define rightMotor1 5


Adafruit_MCP23X17 mcp;
// Servo clawServo;
// Servo auxServo;
int dly = 250;
// int clawServoValue = 90;
// int auxServoValue = 90;
int player = 0;
int battery = 0;
// int servoDelay = 0;

bool cabLightsOn = false;
bool auxLightsOn = false;
// bool moveClawServoUp = false;
// bool moveClawServoDown = false;
// bool moveAuxServoUp = false;
// bool moveAuxServoDown = false;

//added these to detect when we need to "stop pressing" the button and turn off the motor
bool l1 = false;
bool l2 = false;
bool r1 = false;
bool r2 = false;
bool dpad_up = false;
bool dpad_down = false;
bool dpad_right = false;
bool dpad_left = false;
bool button_a = false;
bool button_b = false;
bool button_x = false;
bool button_y = false;
int axisX_prev = 0;
int axisY_prev = 0;
int axisRX_prev = 0;
int axisRY_prev = 0;
int axisX_current = 0;
int axisY_current = 0;
int axisRX_current = 0;
int axisRY_current = 0;
bool stick_L_moving = false;
bool stick_R_moving = false;

//adding this to help with accidental movemet on the joysticks
int threshold = 250;



ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
            cabLightsOn = true; //to signal a controller is ready
            digitalWrite(cabLights, HIGH);
            // Additionally, you can get certain gamepad properties like:
            // Model, VID, PID, BTAddr, flags, etc.
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                           properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not found empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

void dumpGamepad(ControllerPtr ctl) {
    Serial.printf(
        "index=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d,"
        "misc: 0x%02x\n",
        ctl->index(),        // Controller Index
        ctl->dpad(),         // D-pad
        ctl->buttons(),      // bitmask of pressed buttons
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->miscButtons()   // bitmask of pressed "misc" buttons
    );
}

void dumpMouse(ControllerPtr ctl) {
    Serial.printf("idx=%d, buttons: 0x%04x, scrollWheel=0x%04x, delta X: %4d, delta Y: %4d\n",
                   ctl->index(),        // Controller Index
                   ctl->buttons(),      // bitmask of pressed buttons
                   ctl->scrollWheel(),  // Scroll Wheel
                   ctl->deltaX(),       // (-511 - 512) left X Axis
                   ctl->deltaY()        // (-511 - 512) left Y axis
    );
}

void dumpKeyboard(ControllerPtr ctl) {
    static const char* key_names[] = {
        // clang-format off
        // To avoid having too much noise in this file, only a few keys are mapped to strings.
        // Starts with "A", which is offset 4.
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V",
        "W", "X", "Y", "Z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        // Special keys
        "Enter", "Escape", "Backspace", "Tab", "Spacebar", "Underscore", "Equal", "OpenBracket", "CloseBracket",
        "Backslash", "Tilde", "SemiColon", "Quote", "GraveAccent", "Comma", "Dot", "Slash", "CapsLock",
        // Function keys
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
        // Cursors and others
        "PrintScreen", "ScrollLock", "Pause", "Insert", "Home", "PageUp", "Delete", "End", "PageDown",
        "RightArrow", "LeftArrow", "DownArrow", "UpArrow",
        // clang-format on
    };
    static const char* modifier_names[] = {
        // clang-format off
        // From 0xe0 to 0xe7
        "Left Control", "Left Shift", "Left Alt", "Left Meta",
        "Right Control", "Right Shift", "Right Alt", "Right Meta",
        // clang-format on
    };
    Serial.printf("idx=%d, Pressed keys: ", ctl->index());
    for (int key = Keyboard_A; key <= Keyboard_UpArrow; key++) {
        if (ctl->isKeyPressed(static_cast<KeyboardKey>(key))) {
            const char* keyName = key_names[key-4];
            Serial.printf("%s,", keyName);
       }
    }
    for (int key = Keyboard_LeftControl; key <= Keyboard_RightMeta; key++) {
        if (ctl->isKeyPressed(static_cast<KeyboardKey>(key))) {
            const char* keyName = modifier_names[key-0xe0];
            Serial.printf("%s,", keyName);
        }
    }
    Console.printf("\n");
}

void dumpBalanceBoard(ControllerPtr ctl) {
    Serial.printf("idx=%d,  TL=%u, TR=%u, BL=%u, BR=%u, temperature=%d\n",
                   ctl->index(),        // Controller Index
                   ctl->topLeft(),      // top-left scale
                   ctl->topRight(),     // top-right scale
                   ctl->bottomLeft(),   // bottom-left scale
                   ctl->bottomRight(),  // bottom-right scale
                   ctl->temperature()   // temperature: used to adjust the scale value's precision
    );
}

void processGamepad(ControllerPtr ctl) {
    //  a(), b(), x(), y(), l1(), etc...
    if (ctl->a()) {
      Serial.println("A");
    }
    if (ctl->b()) {
      //servo action
      Serial.println("B");
    }
    if (ctl->x()) {
      //servo action
      Serial.println("X");
    }
    if (ctl->y()) {
      //servo action
      Serial.println("Y");
    }
    //------------- Digital shoulder button events -------------
    if (ctl->l1() && ! l1) { //move "second" boom
      l1 = true;
      mcp.digitalWrite(secondBoom0, HIGH);
      mcp.digitalWrite(secondBoom1, LOW);
      // Serial.println("Started pressing the left shoulder button");
    }
    if (! ctl->l1() && l1) { //move "second" boom
      l1 = false;
      mcp.digitalWrite(secondBoom0, LOW);
      mcp.digitalWrite(secondBoom1, LOW);
      // Serial.println("Stopped pressing the left shoulder button");
    }
    if (ctl->r1() && ! r1) {//move main boom
      r1 = true;
      mcp.digitalWrite(mainBoom0, HIGH);
      mcp.digitalWrite(mainBoom1, LOW);
      // Serial.println("Started pressing the right shoulder button");
    }
    if (! ctl->r1() && r1) {//move main boom
      r1 = false;
      mcp.digitalWrite(mainBoom0, LOW);
      mcp.digitalWrite(mainBoom1, LOW);
      // Serial.println("Stopped pressing the right shoulder button");
    }
    //-------------- Digital trigger button  events -------------
    if (ctl->l2() && ! l2) { //move "second" boom
      l2 = true;
      mcp.digitalWrite(secondBoom0, LOW);
      mcp.digitalWrite(secondBoom1, HIGH);
      // Serial.println("Started pressing the left trigger");
    }
    if (! ctl->l2() && l2) { //move "second" boom
      l2 = false;
      mcp.digitalWrite(secondBoom0, LOW);
      mcp.digitalWrite(secondBoom1, LOW);
      // Serial.println("Stopped pressing the left trigger");
    }
    if (ctl->r2() && ! r2) { //move main boom
      r2 = true;
      mcp.digitalWrite(mainBoom0, LOW);
      mcp.digitalWrite(mainBoom1, HIGH);
      // Serial.println("Started pressing the right trigger");
    }
    if (! ctl->r2() && r2) {//move main boom
      r2 = false;
      mcp.digitalWrite(mainBoom0, LOW);
      mcp.digitalWrite(mainBoom1, LOW);
      // Serial.println("Stopped pressing the right trigger");
    }

  //---------------- Analog stick value events ---------------
  axisX_prev = axisX_current;
  axisY_prev = axisY_current;
  axisRX_prev = axisRX_current;
  axisRY_prev = axisRY_current;
  axisX_current = ctl->axisX();
  axisY_current = ctl->axisY();
  axisRX_current = ctl->axisRX();
  axisRY_current = ctl->axisRY();

//L stick Y+ move L+R forward ✅
//        Y- move L+R backward ✅
//        X+ move L forward and R backward ✅
//        X- move R forward and L backward ✅
//        Y+ and X+ move only R forward ✅
//        Y+ and X- move only L forward ✅
//        Y- and X+ move only R backward ✅
//        Y- and X- move only L backward ✅

    if(axisY_current > 0 && axisY_current > threshold && abs(axisX_current) < threshold){ 
      //Y+ move L+R forward 
      //and X axis is not actually being moved much
      mcp.digitalWrite(leftMotor0, HIGH); //L forward
      mcp.digitalWrite(leftMotor1, LOW);
      mcp.digitalWrite(rightMotor0, HIGH); //R forward
      mcp.digitalWrite(rightMotor1, LOW);
    }
    else if(axisY_current < 0 && abs(axisY_current) > threshold && abs(axisX_current) < threshold){ 
      //Y- move L+R backward 
      //and X axis is not actually being moved much
      mcp.digitalWrite(leftMotor0, LOW); //L backward
      mcp.digitalWrite(leftMotor1, HIGH);
      mcp.digitalWrite(rightMotor0, LOW); //R backward
      mcp.digitalWrite(rightMotor1, HIGH);
    }
    else if(axisX_current > 0 && axisX_current > threshold && abs(axisY_current) < threshold){ //rotate
      //X+ move L forward and R backward
      mcp.digitalWrite(leftMotor0, HIGH); //L forward
      mcp.digitalWrite(leftMotor1, LOW);
      mcp.digitalWrite(rightMotor0, LOW);
      mcp.digitalWrite(rightMotor1, HIGH); //R backward
    }
    else if(axisX_current < 0 && abs(axisX_current) > threshold && abs(axisY_current) < threshold){ //rotate
      //X- move R forward and L backward
      mcp.digitalWrite(leftMotor0, LOW); //L backward
      mcp.digitalWrite(leftMotor1, HIGH);
      mcp.digitalWrite(rightMotor0, HIGH); //R forward
      mcp.digitalWrite(rightMotor1, LOW);
    }
    else if(axisY_current > 0 && axisX_current > 0 && axisX_current > threshold && axisY_current > threshold){
      //Y+ and X+ move only R forward
      mcp.digitalWrite(rightMotor0, HIGH); //R forward
      mcp.digitalWrite(rightMotor1, LOW);
    }
    else if(axisY_current > 0 && axisX_current < 0 && abs(axisX_current) > threshold && axisY_current > threshold){
      //Y+ and X- move only L forward
      mcp.digitalWrite(leftMotor0, HIGH); //L forward
      mcp.digitalWrite(leftMotor1, LOW);
    }
    else if(axisY_current < 0 && axisX_current > 0 && axisX_current > threshold && abs(axisY_current) > threshold){
      //Y- and X+ move only R backward
      mcp.digitalWrite(rightMotor0, LOW); //R backward
      mcp.digitalWrite(rightMotor1, HIGH);
    }
    else if(axisY_current < 0 && axisX_current < 0 && abs(axisX_current) > threshold && abs(axisY_current) > threshold){
      //Y- and X- move only L backward
      mcp.digitalWrite(leftMotor0, LOW); //L backward
      mcp.digitalWrite(leftMotor1, HIGH);
    }
    else if(abs(axisX_current) < threshold && abs(axisY_current) < threshold){
      //stop moving!
      mcp.digitalWrite(leftMotor0, LOW);
      mcp.digitalWrite(leftMotor1, LOW);
      mcp.digitalWrite(rightMotor0, LOW);
      mcp.digitalWrite(rightMotor1, LOW);
    }


  //R stick X+ pivot right ✅
  //        X- pivot left ✅
  //        Y+ bucket open ✅
  //        Y- bucket close ✅

  //right stick
    if(abs(axisRX_current) > threshold){ // start pivoting either direction
      if (axisRX_current > threshold) {
        mcp.digitalWrite(pivot0, HIGH);
        mcp.digitalWrite(pivot1, LOW);
        delay(10);
        // Serial.println("Made to into Positive");
      }
      if (axisRX_current < (threshold * -1)) {
        mcp.digitalWrite(pivot0, LOW);
        mcp.digitalWrite(pivot1, HIGH);
        delay(10);
        // Serial.println("Made to into negative");
      }
    }
    else {
      mcp.digitalWrite(pivot0, LOW);
      mcp.digitalWrite(pivot1, LOW);
      delay(10);
      // Serial.println("stop pivot");
    }
    if(abs(axisRY_current) > threshold){ // start tilt bucket either direction
      if (axisRY_current > threshold) {
        mcp.digitalWrite(tiltAttach0, HIGH);
        mcp.digitalWrite(tiltAttach1, LOW);
        delay(10);
        Serial.print("Move bucket positive");
      }
      if (axisRY_current < (threshold * -1)) {
        mcp.digitalWrite(tiltAttach0, LOW);
        mcp.digitalWrite(tiltAttach1, HIGH);
        delay(10);
        Serial.print("Move bucket negative");
      }
    }
    else{ //stop tilt bucket
      mcp.digitalWrite(tiltAttach0, LOW);
      mcp.digitalWrite(tiltAttach1, LOW);
    }

   
  //--------------- Digital D-pad button events --------------
  if (ctl->dpad() == 1) {
    dpad_up = true;
    mcp.digitalWrite(thumb0, HIGH);
    mcp.digitalWrite(thumb1, LOW);
    Serial.println("Started pressing the up button");
  }
  if (dpad_up && ctl->dpad() != 1) {
    dpad_up = false;
    mcp.digitalWrite(thumb0, LOW);
    mcp.digitalWrite(thumb1, LOW);
    Serial.println("Released the up button");
  }
  if (ctl->dpad() == 2) {
    dpad_down = true;
    Serial.println("Started pressing the down button");
    mcp.digitalWrite(thumb0, LOW);
    mcp.digitalWrite(thumb1, HIGH);
  }
  if (dpad_down && ctl->dpad() != 2) {
    dpad_down = false;
    Serial.println("Released the down button");
    mcp.digitalWrite(thumb0, LOW);
    mcp.digitalWrite(thumb1, LOW);
  }
  //--------------- Digital stick button events --------------
  //cab lights
  if (ctl->thumbL()) {
    if (!cabLightsOn) {
      digitalWrite(cabLights, HIGH);
      cabLightsOn = true;
    } else {
      digitalWrite(cabLights, LOW);
      cabLightsOn = false;
    }
    Serial.println("Click left stick button");
  }


    // Another way to query controller data is by getting the buttons() function.
    // See how the different "dump*" functions dump the Controller info.
    // dumpGamepad(ctl);
} //end processGamepad()


void processMouse(ControllerPtr ctl) {
    // This is just an example.
    if (ctl->scrollWheel() > 0) {
        // Do Something
    } else if (ctl->scrollWheel() < 0) {
        // Do something else
    }

    // See "dumpMouse" for possible things to query.
    dumpMouse(ctl);
}

void processKeyboard(ControllerPtr ctl) {
    if (!ctl->isAnyKeyPressed())
        return;

    // This is just an example.
    if (ctl->isKeyPressed(Keyboard_A)) {
        // Do Something
        Serial.println("Key 'A' pressed");
    }

    // Don't do "else" here.
    // Multiple keys can be pressed at the same time.
    if (ctl->isKeyPressed(Keyboard_LeftShift)) {
        // Do something else
        Serial.println("Key 'LEFT SHIFT' pressed");
    }

    // Don't do "else" here.
    // Multiple keys can be pressed at the same time.
    if (ctl->isKeyPressed(Keyboard_LeftArrow)) {
        // Do something else
        Serial.println("Key 'Left Arrow' pressed");
    }

    // See "dumpKeyboard" for possible things to query.
    dumpKeyboard(ctl);
}

void processBalanceBoard(ControllerPtr ctl) {
    // This is just an example.
    if (ctl->topLeft() > 10000) {
        // Do Something
    }

    // See "dumpBalanceBoard" for possible things to query.
    dumpBalanceBoard(ctl);
}

void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            } else if (myController->isMouse()) {
                processMouse(myController);
            } else if (myController->isKeyboard()) {
                processKeyboard(myController);
            } else if (myController->isBalanceBoard()) {
                processBalanceBoard(myController);
            } else {
                Serial.println("Unsupported controller");
            }
        }
    }
}

// Arduino setup function. Runs in CPU 1
void setup() {
    Serial.begin(115200);
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    // Setup the Bluepad32 callbacks
    BP32.setup(&onConnectedController, &onDisconnectedController);

    // "forgetBluetoothKeys()" should be called when the user performs
    // a "device factory reset", or similar.
    // Calling "forgetBluetoothKeys" in setup() just as an example.
    // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
    // But it might also fix some connection / re-connection issues.
    BP32.forgetBluetoothKeys();

    // Enables mouse / touchpad support for gamepads that support them.
    // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
    // - First one: the gamepad
    // - Second one, which is a "virtual device", is a mouse.
    // By default, it is disabled.
    BP32.enableVirtualDevice(false);


    mcp.begin_I2C();
    //   put your setup code here, to run once:


    for (int i = 0; i <= 15; i++) {
      mcp.pinMode(i, OUTPUT);
    }

    Serial.println("Ready.");

    // pinMode(clawServoPin, OUTPUT);
    // pinMode(auxServoPin, OUTPUT);

    pinMode(cabLights, OUTPUT);
    pinMode(auxLights, OUTPUT);
}

// Arduino loop function. Runs in CPU 1.
void loop() {
    // This call fetches all the controllers' data.
    // Call this function in your main loop.
    bool dataUpdated = BP32.update();
    if (dataUpdated)
        processControllers();

    // The main loop must have some kind of "yield to lower priority task" event.
    // Otherwise, the watchdog will get triggered.
    // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
    // Detailed info here:
    // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

    //     vTaskDelay(1);
    delay(150);
}
