/*
 This file is part of Darling.

 Copyright (C) 2019 Lubos Dolezel

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

#include <Foundation/Foundation.h>

typedef double CLLocationAccuracy;
typedef double CLLocationDegrees;

typedef struct CLLocationCoordinate2D
{
    CLLocationDegrees latitude;
    CLLocationDegrees longitude;
} CLLocationCoordinate2D;

#ifdef __cplusplus
extern "C" {
#endif

static inline CLLocationCoordinate2D CLLocationCoordinate2DMake(CLLocationDegrees latitude, CLLocationDegrees longitude)
{
    CLLocationCoordinate2D coordinate;
    coordinate.latitude = latitude;
    coordinate.longitude = longitude;
    return coordinate;
}

// YES when the latitude is within -90...90 and the longitude within -180...180 (both ends included).
extern BOOL CLLocationCoordinate2DIsValid(CLLocationCoordinate2D coord);

#ifdef __cplusplus
}
#endif

extern const CLLocationAccuracy kCLLocationAccuracyBestForNavigation;
extern const CLLocationAccuracy kCLLocationAccuracyBest;
extern const CLLocationAccuracy kCLLocationAccuracyNearestTenMeters;
extern const CLLocationAccuracy kCLLocationAccuracyHundredMeters;
extern const CLLocationAccuracy kCLLocationAccuracyKilometer;
extern const CLLocationAccuracy kCLLocationAccuracyThreeKilometers;

@interface CLLocation : NSObject

@end
