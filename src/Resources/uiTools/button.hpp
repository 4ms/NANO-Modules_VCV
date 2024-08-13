#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "rack.hpp"

using namespace rack;

class ButtonDebounce {
public:
    ButtonDebounce() = default;
    
    // Update the debouncer state; call this in your module's process method
    void update(bool currentButtonState) {
        // Shift the state to the left and add the current state to the least significant bit
        buttonState = (buttonState << 1) | (currentButtonState ? 1 : 0);

        // Update flags based on the current buttonState pattern
        // Here, a sequence of 1's indicates a press and a sequence of 0's indicates no press
        buttonJustPressed = (buttonState == 0x01); // Just pressed condition
        buttonPressed = (buttonState == 0xFF); // Continuously pressed condition
        buttonReleased = (buttonState == 0x80); // Just released condition
    }

    // Returns true if the button was just pressed
    bool isJustPressed() const {
        return buttonJustPressed;
    }

    // Returns true if the button is currently pressed
    bool isPressed() const {
        return buttonPressed;
    }

    // Returns true if the button was just released
    bool isReleased() const {
        return buttonReleased;
    }

private:
    uint8_t buttonState = 0xFF; // Initialize to all 1s to indicate no press initially
    bool buttonJustPressed = false;
    bool buttonPressed = false;
    bool buttonReleased = false;
};

#endif // BUTTON_HPP
