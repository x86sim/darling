// Copyright (C) 2026 Lubos Dolezel
//
// Typecheck of what the Swift CoreLocation and MapKit overlays (apple/swift 5.2.2,
// stdlib/public/Darwin/{CoreLocation,MapKit}/NSValue.swift.gyb) need from the two Clang modules:
// CLLocationCoordinate2D with a zero initializer, the NSValue category selector
// +valueWithMKCoordinate: looked up at run time, NSValue(mkCoordinateSpan:) and
// NSValue.mkCoordinateSpanValue, and MKCoordinateSpan with a zero initializer.
// The bridging conformances themselves are not repeated here so that this file also compiles next
// to the real overlay.
// Build: swiftc -typecheck corelocation_mapkit_import.swift -target x86_64-apple-macosx26.0 -sdk <SDK>

import CoreLocation
import Foundation
import MapKit

// The overlay looks +valueWithMKCoordinate: up through a Selector (ObjectiveC overlay module) and calls it to
// learn the NSValue type encoding; that lookup is checked at run time by tests/src/mapkit_geometry_nsvalue.m.
// Typechecked here is the fallback it uses when the category method is missing.
private let coordinateObjCType: UnsafePointer<CChar> = _getObjCTypeEncoding(CLLocationCoordinate2D.self)

private let spanObjCType = NSValue(mkCoordinateSpan: .init()).objCType

func coordinateValue(_ c: CLLocationCoordinate2D) -> NSValue {
  var addressable = c
  return NSValue(bytes: &addressable, objCType: coordinateObjCType)
}

func coordinate(from value: NSValue) -> CLLocationCoordinate2D {
  var result = CLLocationCoordinate2D()
  value.getValue(&result)
  return result
}

func useGeometry() -> Bool {
  let c = CLLocationCoordinate2DMake(1.5, 2.5)
  let back = coordinate(from: coordinateValue(c))
  let span = MKCoordinateSpan(latitudeDelta: 3.5, longitudeDelta: 4.5)
  let spanValue = NSValue(mkCoordinateSpan: span)
  let region = MKCoordinateRegion(center: c, span: spanValue.mkCoordinateSpanValue)
  return strcmp(spanValue.objCType, spanObjCType) == 0
    && back.latitude == c.latitude
    && region.span.longitudeDelta == 4.5 && region.center.longitude == 2.5
    && MKCoordinateSpanMake(1, 2).latitudeDelta == 1
    && MKCoordinateRegionMake(c, span).span.latitudeDelta == 3.5
}
_ = CLLocationCoordinate2DIsValid(CLLocationCoordinate2DMake(0, 0))
_ = useGeometry()
