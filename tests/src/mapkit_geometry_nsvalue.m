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


// CoreLocation / MapKit geometry: the coordinate and span structs, their Make functions,
// CLLocationCoordinate2DIsValid, and the NSValue category used by the Swift overlays.
// Prints PASS when everything holds; the exit code is the number of failed checks.

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>
#import <MapKit/MapKit.h>
#include <math.h>
#include <string.h>

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

static void test_structs(void)
{
    CHECK(sizeof(CLLocationDegrees) == 8);
    CHECK(sizeof(CLLocationCoordinate2D) == 16 && sizeof(MKCoordinateSpan) == 16 && sizeof(MKCoordinateRegion) == 32);

    CLLocationCoordinate2D c = CLLocationCoordinate2DMake(1.5, -2.5);
    CHECK(c.latitude == 1.5 && c.longitude == -2.5);
    MKCoordinateSpan s = MKCoordinateSpanMake(3.5, 4.5);
    CHECK(s.latitudeDelta == 3.5 && s.longitudeDelta == 4.5);
    MKCoordinateRegion r = MKCoordinateRegionMake(c, s);
    CHECK(r.center.latitude == 1.5 && r.center.longitude == -2.5 && r.span.latitudeDelta == 3.5 && r.span.longitudeDelta == 4.5);
    // Members are laid out in declaration order.
    double raw[2];
    memcpy(raw, &c, sizeof(raw));
    CHECK(raw[0] == 1.5 && raw[1] == -2.5);
}

static void test_is_valid(void)
{
    // Range limits are inclusive; NaN and infinity are not valid. Documented: latitude -90...90, longitude -180...180.
    static const struct { double lat, lon; int valid; } cases[] = {
        { 0, 0, 1 }, { 90, 180, 1 }, { -90, -180, 1 }, { 45.5, -122.5, 1 },
        { 90.0000001, 0, 0 }, { 0, 180.0000001, 0 }, { -91, 0, 0 }, { 0, -181, 0 },
        { NAN, 0, 0 }, { 0, NAN, 0 }, { INFINITY, 0, 0 }, { 0, -INFINITY, 0 },
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
        CHECK(CLLocationCoordinate2DIsValid(CLLocationCoordinate2DMake(cases[i].lat, cases[i].lon)) == cases[i].valid);
}

static void test_nsvalue(void)
{
    @autoreleasepool
    {
        NSValue *cv = [NSValue valueWithMKCoordinate:CLLocationCoordinate2DMake(1.5, 2.5)];
        CHECK(strcmp([cv objCType], "{CLLocationCoordinate2D=dd}") == 0);
        CLLocationCoordinate2D back = cv.MKCoordinateValue;
        CHECK(back.latitude == 1.5 && back.longitude == 2.5);

        NSValue *sv = [NSValue valueWithMKCoordinateSpan:MKCoordinateSpanMake(3.5, 4.5)];
        // test_quirk_span_encoding_is_anonymous_struct: MKCoordinateSpan is an unnamed struct typedef, so
        // its type encoding is {?=dd}; the Swift overlay reads the encoding back from an NSValue.
        CHECK(strcmp([sv objCType], "{?=dd}") == 0);
        CHECK(strcmp(@encode(MKCoordinateSpan), "{?=dd}") == 0);
        CHECK(strcmp(@encode(MKCoordinateRegion), "{?={CLLocationCoordinate2D=dd}{?=dd}}") == 0);
        MKCoordinateSpan sback = sv.MKCoordinateSpanValue;
        CHECK(sback.latitudeDelta == 3.5 && sback.longitudeDelta == 4.5);

        // A value built from the struct's own encoding reads back through the accessor.
        CLLocationCoordinate2D src = CLLocationCoordinate2DMake(1, 2);
        NSValue *raw = [NSValue valueWithBytes:&src objCType:@encode(CLLocationCoordinate2D)];
        CHECK(raw.MKCoordinateValue.latitude == 1 && raw.MKCoordinateValue.longitude == 2);
        CHECK([cv isEqual:[NSValue valueWithMKCoordinate:CLLocationCoordinate2DMake(1.5, 2.5)]]);
        CHECK(![cv isEqual:sv]);

        // The Swift overlay finds the coordinate factory by selector at run time.
        CHECK([NSValue respondsToSelector:@selector(valueWithMKCoordinate:)]);
        CHECK([NSValue respondsToSelector:@selector(valueWithMKCoordinateSpan:)]);
        CHECK([cv respondsToSelector:@selector(MKCoordinateValue)] && [sv respondsToSelector:@selector(MKCoordinateSpanValue)]);
    }
}

int main(void)
{
    test_structs();
    test_is_valid();
    test_nsvalue();
    if (failures == 0)
        printf("PASS\n");
    return failures;
}
