#import "MBXAnnotationView.h"

@interface MBXAnnotationView ()

@property (nonatomic) UIView *centerView;

@end

@implementation MBXAnnotationView

- (void)layoutSubviews {
    [super layoutSubviews];
    if (!self.centerView) {
        self.backgroundColor = [UIColor blueColor];
        self.centerView = [[UIView alloc] initWithFrame:CGRectInset(self.bounds, 1.0, 1.0)];
        self.centerView.backgroundColor = self.centerColor;
        [self addSubview:self.centerView];
    }
}

- (void)setCenterColor:(UIColor *)centerColor {
    if (![_centerColor isEqual:centerColor]) {
        _centerColor = centerColor;
        self.centerView.backgroundColor = centerColor;
    }
}

- (nullable id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    if ([event isEqualToString:@"position"])
    {
        CABasicAnimation *animation = [CABasicAnimation animationWithKeyPath:event];
        animation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
        animation.speed = 0.1;
        return animation;
    }
    return [super actionForLayer:layer forKey:event];
}

@end
