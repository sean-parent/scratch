//
//  ViewController.m
//  implies
//
//  Created by Sean Parent on 10/23/17.
//  Copyright ¬© 2017 Sean Parent. All rights reserved.
//

#import "ViewController.h"

#include <cstddef>

#if 0
#include <adobe/adam.hpp>
#include <adobe/adam_parser.hpp>
#endif

@interface ViewController ()

@property (nonatomic) IBOutlet UISwitch *aSwitch;
@property (nonatomic) IBOutlet UISwitch *bSwitch;
@property (nonatomic) IBOutlet UIButton *operation;
@property (nonatomic) IBOutlet UIView *indicator;

@end

@implementation ViewController {
    bool _a;
    bool _b;

    std::size_t _example;
}

- (IBAction)aChanged {
    switch (_example) {
        case 1:
            break;
        case 2:
            if (_aSwitch.on) _bSwitch.on = true;
            break;
        case 3:
            _bSwitch.enabled = !_aSwitch.on;
            if (_aSwitch.on) _bSwitch.on = true;
            break;
        case 4:
            _bSwitch.enabled = !_aSwitch.on;
            _bSwitch.on = _aSwitch.on || _b;
            break;
        case 5:
            _a = _aSwitch.on;
            _bSwitch.on = _a || _b;
            break;
        case 6:
            _bSwitch.on = _aSwitch.on || _bSwitch.on;
            break;
        case 7:
            _operation.enabled = !_aSwitch.on || _bSwitch.on;
            break;
        default:;
    }
}

- (IBAction)bChanged {
    switch (_example) {
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            _b = _bSwitch.on;
            break;
        case 5:
            _b = _bSwitch.on;
            _aSwitch.on = _b && _a;
            break;
        case 6:
            _aSwitch.on = _bSwitch.on && _aSwitch.on;
            break;
        case 7:
            _operation.enabled = !_aSwitch.on || _bSwitch.on;
            break;
        default:;
    }
}

- (IBAction)onOperation {
    bool a = _aSwitch.on;
    bool b = _bSwitch.on;

    switch (_example) {
        case 1:
            b = a || b;
            break;
        default:;
    }

    bool valid = !a || b;

    [UIView animateWithDuration:0.5 animations:^{
        _indicator.backgroundColor = valid ? [UIColor greenColor] : [UIColor redColor];
    } completion:^(BOOL finished){
        [UIView animateWithDuration:0.5 animations:^{
            _indicator.backgroundColor = [UIColor whiteColor];
        }];
    }];

}

- (IBAction)exampleChanged:(UISegmentedControl *)source {
    _example = source.selectedSegmentIndex + 1;

    _a = false;
    _b = false;

    _aSwitch.on = _a;
    _bSwitch.on = _b;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    _example = 1;

    _aSwitch.on = _a;
    _bSwitch.on = _b;
}

@end

