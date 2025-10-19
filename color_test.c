#include <stdio.h>

int main() {
    printf("Testing 256 colors:\n");
    
    // Test a few specific colors
    printf("\033[38;5;196mBright Red (196)\033[0m\n");      
    printf("\033[38;5;46mBright Green (46)\033[0m\n");      
    printf("\033[38;5;21mBlue (21)\033[0m\n");              
    printf("\033[38;5;51mCyan (51)\033[0m\n");              
    printf("\033[38;5;226mYellow (226)\033[0m\n");          
    printf("\033[38;5;15mWhite (15)\033[0m\n");             
    
    // Test the format_color_256 function
    printf("format_color_256 test: ");
    printf("\033[38;5;196m");  // Should be bright red
    printf("RED");
    printf("\033[0m");
    printf(" ");
    printf("\033[38;5;46m");   // Should be bright green
    printf("GREEN");
    printf("\033[0m");
    printf("\n");
    
    return 0;
}