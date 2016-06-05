#import <UIKit/UIKit.h>

#import "MGLTypes.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Options for locking the orientation of an `MGLAnnotationView` along one or more
 axes for a billboard effect.
 */
typedef NS_OPTIONS(NSUInteger, MGLAnnotationViewBillboardAxis)
{
    /**
     Orients the annotation view such that its x-axis is always fixed with
     respect to the map.
     
     If this option is unset, the annotation view remains unchanged as the map’s
     pitch increases, so that the view appears to stand upright on the tilted
     map. If this option is set, the annotation view tilts as the map’s pitch
     increases, so that the view appears to lie flat against the tilted map.
     
     For example, you would set this option if the annotation view depicts an
     arrow that should always point due south. You would unset this option if
     the arrow should always point down towards the ground.
     */
    MGLAnnotationViewBillboardAxisX = 0x1 << 0,
    
    /**
     Orients the annotation view such that its y-axis is always fixed with
     respect to the map.
     
     If this option is unset, the annotation view remains unchanged as the map
     is rotated. If this option is set, the annotation view rotates as the map
     rotates.
     
     For example, you would set this option if the annotation view should be
     aligned with a street, regardless of the direction from which the user
     views the street.
     */
    MGLAnnotationViewBillboardAxisY = 0x1 << 1,
    
    /**
     Orients the annotation view such that its z-axis is always fixed with
     respect to the map.
     
     Because `MGLMapView` does not support changes to its bank, or roll, this
     option has no effect.
     */
    MGLAnnotationViewBillboardAxisZ = 0x1 << 2,
    
    /**
     Orients the annotation view such that all three axes are always fixed with
     respect to the map.
     */
    MGLAnnotationViewBillboardAxisAll = (MGLAnnotationViewBillboardAxisX | MGLAnnotationViewBillboardAxisY | MGLAnnotationViewBillboardAxisZ),
};

/**
 The `MGLAnnotationView` class is responsible for representing point-based
 annotation markers as a view. Annotation views represent an annotation object,
 which is an object that corresponds to the `MGLAnnotation` protocol. When an
 annotation’s coordinate point is visible on the map view, the map view delegate
 is asked to provide a corresponding annotation view. If an annotation view is
 created with a reuse identifier, the map view may recycle the view when it goes
 offscreen.
 */
@interface MGLAnnotationView : UIView

/**
 Initializes and returns a new annotation view object.
 
 @param reuseIdentifier The string that identifies that this annotation view is
    reusable.
 @return The initialized annotation view object.
 */
- (instancetype)initWithReuseIdentifier:(nullable NSString *)reuseIdentifier;

/**
 The string that identifies that this annotation view is reusable.
 
 You specify the reuse identifier when you create the view. You use the
 identifier later to retrieve an annotation view that was created previously but
 which is currently unused because its annotation does not lie within the map
 view’s viewport.
 
 If you define distinctly different types of annotations (with distinctly
 different annotation views to go with them), you can differentiate between the
 annotation types by specifying different reuse identifiers for each one.
 */
@property (nonatomic, readonly, nullable) NSString *reuseIdentifier;

/**
 The offset from the annotation’s logical center to the annotation view’s visual
 center point.
 
 The offset is measured in points. A positive offset moves the annotation view
 towards the bottom-right, while a negative offset moves it towards the
 top-left.
 
 By default, there is no offset, so the view is perfectly centered around the
 position on-screen that corresponds to the annotation object’s `coordinate`
 property.
 
 Set the offset if the annotation view’s visual center point is somewhere other
 than the center of the view. For example, the view may contain an image that
 depicts a downward-pointing pushpin or thumbtack, with the tip positioned at
 the center-bottom of the view. In that case, you would set the offset’s `dx` to
 zero and its `dy` to half the height of the view.
 */
@property (nonatomic) CGVector centerOffset;

/**
 An option that specifies the annotation view’s degrees of freedom.
 
 By default, none of the axes are free; in other words, the annotation view is
 oriented like a billboard with respect to the x-, y-, and z-axes. See
 `MGLAnnotationViewBillboardAxis` for available options.
 */
@property (nonatomic, assign) MGLAnnotationViewBillboardAxis freeAxes;

/**
 A Boolean value that determines whether the annotation view’s scale changes as
 the distance between the viewpoint and the annotation view changes on a tilted
 map.
 
 When the value of this property is `YES` and the map is tilted, the annotation
 view appears smaller if it is towards the top of the view (closer to the
 horizon) and larger if it is towards the bottom of the view (closer to the
 viewpoint). This is also the behavior of `MGLAnnotationImage` objects. When the
 value of this property is `NO` or the map’s pitch is zero, the annotation view
 remains the same size regardless of its position on-screen.
 
 The default value of this property is `YES`. Set this property to `NO` if the
 view’s legibility is important.
 */
@property (nonatomic, assign) BOOL scalesWithViewingDistance;

/**
 Called when the view is removed from the reuse queue.

 The default implementation of this method does nothing. You can override it in
 your custom annotation views and use it to put the view in a known state before
 it is returned to your map view delegate.
 */
- (void)prepareForReuse;

@end

NS_ASSUME_NONNULL_END
