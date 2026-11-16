/*
 This file is part of Darling.

 Copyright (C) 2026 Lubos Dolezel

 Darling is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Darling is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Darling.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MKGeometry_H_
#define _MKGeometry_H_

#import <Foundation/Foundation.h>
#import <CoreLocation/CLLocation.h>

#ifdef __cplusplus
extern "C" {
#endif

// The size of a map region in degrees of latitude and longitude.
// QUIRK: MKCoordinateSpan and MKCoordinateRegion are unnamed struct typedefs, so their type encodings are
// {?=dd} and {?={CLLocationCoordinate2D=dd}{?=dd}} (CLLocationCoordinate2D itself is a named struct); the
// Swift overlay compares NSValue encodings against these. Not documented. Evidence: black-box observation
// on macOS 26.7.3 x86_64 (tier 2). Test: test_quirk_span_encoding_is_anonymous_struct in
// tests/src/mapkit_geometry_nsvalue.m
typedef struct
{
    CLLocationDegrees latitudeDelta;
    CLLocationDegrees longitudeDelta;
} MKCoordinateSpan;

// A region of the map: its center coordinate and the span around it.
typedef struct
{
    CLLocationCoordinate2D center;
    MKCoordinateSpan span;
} MKCoordinateRegion;

static inline MKCoordinateSpan MKCoordinateSpanMake(CLLocationDegrees latitudeDelta, CLLocationDegrees longitudeDelta)
{
    MKCoordinateSpan span;
    span.latitudeDelta = latitudeDelta;
    span.longitudeDelta = longitudeDelta;
    return span;
}

static inline MKCoordinateRegion MKCoordinateRegionMake(CLLocationCoordinate2D centerCoordinate, MKCoordinateSpan span)
{
    MKCoordinateRegion region;
    region.center = centerCoordinate;
    region.span = span;
    return region;
}

#ifdef __cplusplus
}
#endif

NS_ASSUME_NONNULL_BEGIN

@interface NSValue (MKGeometryAdditions)

+ (instancetype)valueWithMKCoordinate:(CLLocationCoordinate2D)coordinate;
+ (instancetype)valueWithMKCoordinateSpan:(MKCoordinateSpan)span;
@property (readonly) CLLocationCoordinate2D MKCoordinateValue;
@property (readonly) MKCoordinateSpan MKCoordinateSpanValue;

@end

NS_ASSUME_NONNULL_END

#endif
