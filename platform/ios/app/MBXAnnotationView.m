#import "MBXAnnotationView.h"

@interface MBXAnnotationView ()
@end

@implementation MBXAnnotationView

- (void)layoutSubviews {
    [super layoutSubviews];
    
    self.layer.borderColor = [UIColor blueColor].CGColor;
    self.layer.borderWidth = 1;
    self.layer.cornerRadius = 2;
}

- (nullable id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    if ([event isEqualToString:@"transform"] || [event isEqualToString:@"position"])
    {
        CABasicAnimation *animation = [CABasicAnimation animationWithKeyPath:event];
        animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
        animation.speed = 0.1;
        return animation;
    }
    return [super actionForLayer:layer forKey:event];
}

@end
