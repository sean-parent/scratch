//
//  ViewController.m
//  scratch-ios-view
//
//  Created by Sean Parent on 11/18/14.
//  Copyright (c) 2014 STLab. All rights reserved.
//

#import "ViewController.h"
#include <stdexcept>

@interface ViewController ()

@property (nonatomic) BOOL value0;
@property (nonatomic) BOOL value1;

@property (nonatomic) IBOutlet UISwitch* switch0;
@property (nonatomic) IBOutlet UISwitch* switch1;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    _value1 = !_value0;

    [self addObserver:self forKeyPath:@"value0"
                  options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                  context:nullptr];

    [self addObserver:self forKeyPath:@"value1"
                  options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                  context:nullptr];
}

+ (NSSet *)keyPathsForValuesAffectingValue0 {
    return [NSSet setWithObjects:@"value1", nil];
}

+ (NSSet *)keyPathsForValuesAffectingValue1 {
    return [NSSet setWithObjects:@"value0", nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {

    if (object != self) return;

    auto value = [(NSNumber*)[change objectForKey:NSKeyValueChangeNewKey] boolValue];

    if ([keyPath isEqual:@"value0"]) {
        _switch0.on = value;
        // _value1 = !value;
    } else if ([keyPath isEqual:@"value1"]) {
        _switch1.on = value;
        // _value0 = !value;
    }
}

- (void)setValue0:(BOOL)value {
    _value0 = value;
    _value1 = !value;
}

- (void)setValue1:(BOOL)value {
    _value0 = !value;
    _value1 = value;
}

- (IBAction)switchChanged0:(UISwitch*)sender {
    self.value0 = sender.on;
}

- (IBAction)switchChanged1:(UISwitch*)sender {
    self.value1 = sender.on;
    throw std::runtime_error("");
}

@end






#if 0 // GOOD

#import "ViewController.h"

@interface ViewController ()

@property (nonatomic) BOOL value;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.

    [self addObserver:self forKeyPath:@"value"
                  options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
                  context:nullptr];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
 
    if ((object == self) && [keyPath isEqual:@"value"]) {
        auto value = [(NSNumber*)[change objectForKey:NSKeyValueChangeNewKey] boolValue];
        _switch0.on = value;
        _switch1.on = value;
    }
}

- (void)switchChanged:(UISwitch*)sender {
    self.value = sender.on;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end

#endif





#if 0
#import "ViewController.h"

@interface ViewController ()

@property (nonatomic) BOOL value;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.

    [self addObserver:self
           forKeyPath:@"value"
              options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
              context:nullptr];

    [_switch0 addObserver:self
               forKeyPath:@"on"
                  options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
                  context:nullptr];

    [_switch1 addObserver:self
               forKeyPath:@"on"
                  options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
                  context:nullptr];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
 
    auto value = [(NSNumber*)[change objectForKey:NSKeyValueChangeNewKey] boolValue];

    if ((object == self) && [keyPath isEqual:@"value"]) {
        _switch0.on = value;
        _switch1.on = value;
    } else if ((object == _switch0 || object == _switch1) && [keyPath isEqual:@"on"]) {
        self.value = value;
    }
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
#endif
