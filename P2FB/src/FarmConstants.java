public class FarmConstants {

    // Commands sent to PIC (Java → PIC)
    public static final String CMD_INITIALIZE      = "INITIALIZE:";     // Default initialization command
    public static final String CMD_GET_PRODUCTS    = "GET_PRODUCTS";    // Request graph data
    public static final String CMD_GET_ANIMALS     = "GET_ANIMALS";     // Request animals and their state
    public static final String CMD_RESET           = "RESET";           // Reset command
    public static final String CMD_CONSUME         = "CONSUME:";        // Consume products
    public static final String CMD_SLEEP           = "SLEEP:";          // Put animal to sleep
    public static final String CMD_START_REBELLION = "START_REBELLION"; // Start rebellion mode
    public static final String CMD_STOP_REBELLION  = "STOP_REBELLION";  // Stop rebellion mode

    // Commands received from PIC (PIC → Java)
    public static final String CMD_DATA_PRODUCTS      = "DATA_PRODUCTS:";      // Graph data sample prefix
    public static final String CMD_SLEEP_SUCCESSFUL   = "SLEEP_SUCCESSFUL";    // Animal slept successfully
    public static final String CMD_SLEEP_UNSUCCESSFUL = "SLEEP_UNSUCCESSFUL";  // Animal failed to sleep
    public static final String CMD_DATA_ANIMAL        = "DATA_ANIMALS:";       // Animal data prefix
    public static final String CMD_FINISH             = "FINISH";              // End of data transmission


    // Joystick navigation commands (configurable)
    public static final String CMD_UP     = "MOVE_UP";               // Joystick UP
    public static final String CMD_DOWN   = "MOVE_DOWN";             // Joystick DOWN
    public static final String CMD_LEFT   = "MOVE_LEFT";             // Joystick LEFT
    public static final String CMD_RIGHT  = "MOVE_RIGHT";            // Joystick RIGHT
    public static final String CMD_SELECT = "SELECT";                // Joystick SELECT (pressed)
}
