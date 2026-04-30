#include <iostream>
#include <pigpio.h>
#include <stdexcept>
#include <chrono>
#include <vector>

class GPIODevice
{
    // Make universial GPIO class to inherit
};

// Represents a physical button connected to a GPIO input pin
class Button
{
private:
    bool state; // Last read state of the button
    int pinNum; // GPIO pin number

public:
    Button(int pinNum) : pinNum(pinNum), state(false)
    {
        gpioSetMode(pinNum, PI_INPUT);
        std::cout << "Button initialized on pin " << pinNum << std::endl;
    }

    bool getState() { return state; }
    int getPinNum() { return pinNum; }

    // Reads the physical pin and updates internal state
    // Returns 0 when pressed, 1 when released (pull-up configuration)
    bool readState()
    {
        state = gpioRead(pinNum);
        return state;
    }

    ~Button()
    {
        std::cout << "Button on pin " << pinNum << " destroyed" << std::endl;
    }
};

// Represents a physical LED connected to a GPIO output pin
class LED
{
private:
    bool lightState; // Current on/off state
    int pin;         // GPIO pin number

public:
    LED(int pin) : pin(pin), lightState(false)
    {
        gpioSetMode(pin, PI_OUTPUT);
        std::cout << "LED initialized on pin " << pin << std::endl;
    }

    bool getState() { return lightState; }

    void turnOn()
    {
        gpioWrite(pin, 1);
        lightState = true;
    }

    void turnOff()
    {
        gpioWrite(pin, 0);
        lightState = false;
    }

    ~LED()
    {
        // gpioWrite(pin, 0); // Ensure LED is off on cleanup
        std::cout << "LED on pin " << pin << " destroyed" << std::endl;
    }
};

// Knock Pattern Recognizer
class KnockDetector
{
private:
    std::vector<int64_t> secretPattern;                       // Secret Pattern
    std::vector<int64_t> attemptedPattern;                    // Attempted pattern vector
    bool isRecording;                                         // Current Recording state
    std::chrono::steady_clock::time_point lastKnockTimestamp; // Tracks when the last knock occurred
    static constexpr int64_t KNOCK_THRESHOLD_MS = 2000;       // Timeout threshold
    static constexpr int64_t KNOCK_TOLERANCE_MS = 300;
    bool hasKnocked = false;

public:
    KnockDetector() : isRecording(false) {}

    bool hasKnockedState()
    {
        return hasKnocked;
    }

    // Records a knock and stores the time gap (in ms) between consecutive knocks into secretPattern
    void recordKnock(std::chrono::steady_clock::time_point currentTimestamp, bool isSecret)
    {
        using SteadyClock = std::chrono::steady_clock;

        // If lastKnockTimestamp is still at default
        if (lastKnockTimestamp == std::chrono::time_point<SteadyClock>{})
        {
            lastKnockTimestamp = currentTimestamp; // never been set
            return;                                // Skip gap calculation
        }

        // Calculate difference between current timestamp and last knock timestamp
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimestamp - lastKnockTimestamp).count();

        if (isSecret)
        {
            secretPattern.push_back(diff); // Add difference to secret pattern vector
        }
        else
        {

            attemptedPattern.push_back(diff); // Add difference to attempted vector
        }
        lastKnockTimestamp = currentTimestamp; // Assign current timestamp to the last knock
        hasKnocked = true;
    }

    // Stops recording patterns and prints stored pattern gaps
    void finalizePattern()
    {
        isRecording = false;
        std::cout << "Stored Gap Pattern: ";
        for (const auto &i : secretPattern)
        {
            std::cout << i << " ";
        }

        std::cout << std::endl;
    }

    // Return current timestamp and return true if gap exceeds threshold
    bool hasTimedOut(std::chrono::steady_clock::time_point currentTimestamp)
    {
        // Calculate difference between current timestamp and last knock timestamp
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimestamp - lastKnockTimestamp).count();

        return diff >= KNOCK_THRESHOLD_MS;
    }

    // clears lastKnockTimestamp back to its default
    void reset()
    {
        lastKnockTimestamp = std::chrono::steady_clock::time_point{};
        hasKnocked = false;
    }

    // Returns if patterns match
    bool comparePatterns()
    {
        int numKnocks;
        size_t attemptedPatternSize = attemptedPattern.size();
        size_t secretPatternSize = secretPattern.size();

        if (attemptedPatternSize == secretPatternSize)
        {
        }
    }
};

// Helper Functions
void recordPhase(Button &btn, KnockDetector &detector);
void attemptPhase(Button &btn, KnockDetector &detector);
bool comparePhase(KnockDetector &detector, LED &led);

int main()
{
    // Initialize pigpio daemon — must happen before any GPIO calls
    if (gpioInitialise() < 0)
    {
        throw std::runtime_error("pigpio initialization failed");
    }

    LED led(17);
    Button btn(18);
    KnockDetector knockDetector;

    bool match;
    // Loop for detecting knock transitions
    bool previousState = btn.getState(); // hold previous state
    int knockNum = 0;
    while (true)
    {
        bool currentState = (btn.readState() == 0);
        // detect knock transitions
        if (previousState == 0 && currentState == 1)
        {
            knockNum++;
            std::cout << "Knock no: " << knockNum << " detected!" << std::endl;

            knockDetector.recordKnock(std::chrono::steady_clock::now(), true);
        }
        previousState = currentState;
        auto currentTimeStamp = std::chrono::steady_clock::now();
        // record timestamps
        if (knockDetector.hasKnockedState())
        {
            if (knockDetector.hasTimedOut(currentTimeStamp))
            {
                knockDetector.finalizePattern();
                break;
            }
        }
    }

    knockDetector.reset(); // reset timestamps
    previousState = btn.getState();
    knockNum = 0;

    while (true)
    {
        // read current button state (same as phase 1)
        bool currentState = (btn.readState() == 0);
        // detect knock transition (same as phase 1)
        if (previousState == 0 && currentState == 1)
        {
            knockNum++;
            std::cout << "Knock no: " << knockNum << " detected!" << std::endl;

            knockDetector.recordKnock(std::chrono::steady_clock::now(), false);
        }
        previousState = currentState; // update previousState

        auto currentTimeStamp = std::chrono::steady_clock::now(); // get current timestamp

        // if hasKnockedState, check for timeout
        if (knockDetector.hasKnockedState())
        {
            if (knockDetector.hasTimedOut(currentTimeStamp))
            {
                knockDetector.finalizePattern();
                break;
            }
        }
    }

    // after phase 2 loop:
    // compare patterns — we'll write this next
    // if match: led.turnOn()
    // if no match: led.turnOff()

    led.turnOff();
    gpioTerminate();
}
