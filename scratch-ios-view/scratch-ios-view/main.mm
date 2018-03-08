//
//  main.m
//  scratch-ios-view
//
//  Created by Sean Parent on 11/18/14.
//  Copyright (c) 2014 STLab. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#include <iostream>

int main(int argc, char * argv[]) {
    @autoreleasepool {
        while (true) {
            try {
                    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
            } catch(...) {
                std::cout << "keep going!" << std::endl;
            }
        }
    }
}
