#import <Cocoa/Cocoa.h>

void SetOpenPackageShortcut(void* item)
{
    NSMenuItem* menuItem = (NSMenuItem*)item;
    menuItem.keyEquivalent = @"o";
    menuItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;
}
