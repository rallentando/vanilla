#import <AppKit/AppKit.h>

void disableWindowTabbing(){
    if([NSWindow respondsToSelector:@selector(allowsAutomaticWindowTabbing)]) {
        NSWindow.allowsAutomaticWindowTabbing = NO;
    }
}
