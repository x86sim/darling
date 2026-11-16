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

#import <MapKit/MKGeometry.h>

#include <stdlib.h>
#include <string.h>

// Reads a struct of the given size out of the value. A value that holds fewer bytes than the struct
// (a different type) fills the leading part only, so the read never runs past the value's storage.
static void MKGetValueBytes(NSValue *value, void *buffer, NSUInteger size)
{
    NSUInteger storedSize = 0;
    NSGetSizeAndAlignment([value objCType], &storedSize, NULL);
    if (storedSize >= size)
    {
        [value getValue:buffer];
        return;
    }
    memset(buffer, 0, size);
    if (storedSize > 0)
    {
        void *temp = malloc(storedSize);
        [value getValue:temp];
        memcpy(buffer, temp, storedSize);
        free(temp);
    }
}

@implementation NSValue (MKGeometryAdditions)

+ (NSValue *)valueWithMKCoordinate:(CLLocationCoordinate2D)coordinate
{
    return [NSValue valueWithBytes:&coordinate objCType:@encode(CLLocationCoordinate2D)];
}

+ (NSValue *)valueWithMKCoordinateSpan:(MKCoordinateSpan)span
{
    return [NSValue valueWithBytes:&span objCType:@encode(MKCoordinateSpan)];
}

- (CLLocationCoordinate2D)MKCoordinateValue
{
    CLLocationCoordinate2D coordinate;
    MKGetValueBytes(self, &coordinate, sizeof(coordinate));
    return coordinate;
}

- (MKCoordinateSpan)MKCoordinateSpanValue
{
    MKCoordinateSpan span;
    MKGetValueBytes(self, &span, sizeof(span));
    return span;
}

@end
