// Copyright (C) 2026 Lubos Dolezel
//
// Typecheck of what the Swift GLKit overlay (apple/swift 5.2.2, stdlib/public/Darwin/GLKit/GLKMath.swift.gyb)
// needs from the GLKit module: the overlay adds a Float subscript to GLKMatrix2/3/4, GLKVector2/3/4 and
// GLKQuaternion by loading Floats out of the imported C aggregates. This file does the same through
// differently named helpers (so it also compiles next to the real overlay) and calls the C functions
// from Swift with the signatures the framework header imports.
// Build: swiftc -typecheck glkit_import.swift -target x86_64-apple-macosx26.0 -sdk <SDK>

import GLKit

@inline(__always)
func element<A>(_ aggregate: UnsafePointer<A>, _ index: Int) -> Float {
  return UnsafeRawPointer(aggregate).load(fromByteOffset: index * MemoryLayout<Float>.stride, as: Float.self)
}

func elem2(_ m: GLKMatrix2, _ i: Int) -> Float { var c = m; return element(&c, i) }
func elem3(_ m: GLKMatrix3, _ i: Int) -> Float { var c = m; return element(&c, i) }
func elem4(_ m: GLKMatrix4, _ i: Int) -> Float { var c = m; return element(&c, i) }
func elemV2(_ v: GLKVector2, _ i: Int) -> Float { var c = v; return element(&c, i) }
func elemV3(_ v: GLKVector3, _ i: Int) -> Float { var c = v; return element(&c, i) }
func elemV4(_ v: GLKVector4, _ i: Int) -> Float { var c = v; return element(&c, i) }
func elemQ(_ q: GLKQuaternion, _ i: Int) -> Float { var c = q; return element(&c, i) }

func useMath() -> Float {
  var sum: Float = 0
  let t = GLKMatrix4MakeTranslation(1, 2, 3)
  let r = GLKMatrix4Multiply(t, GLKMatrix4MakeXRotation(0.5))
  var invertible = false
  let inverse = GLKMatrix4Invert(r, &invertible)
  sum += elem4(inverse, 12) + elem4(GLKMatrix4Identity, 0) + elem3(GLKMatrix3Identity, 4)
  sum += elem3(GLKMatrix4GetMatrix3(r), 2) + elem2(GLKMatrix4GetMatrix2(r), 3)
  let v = GLKVector3Make(1, 2, 3)
  sum += elemV3(GLKMatrix4MultiplyVector3(r, v), 1) + elemV2(GLKVector2Make(1, 2), 1)
  sum += elemV4(GLKVector4Make(1, 2, 3, 4), 3)
  let q = GLKQuaternionMakeWithAngleAndAxis(1, 0, 0, 1)
  sum += elemQ(GLKQuaternionSlerp(GLKQuaternionIdentity, q, 0.5), 3) + GLKQuaternionAngle(q)
  sum += GLKVector3DotProduct(v, GLKQuaternionAxis(q)) + GLKVector3Length(v)
  sum += GLKMathDegreesToRadians(90)
  var viewport: [Int32] = [0, 0, 100, 100]
  let p = GLKMathProject(v, t, GLKMatrix4MakePerspective(1, 1, 0.1, 10), &viewport)
  var ok = false
  let u = GLKMathUnproject(p, t, GLKMatrix4MakePerspective(1, 1, 0.1, 10), &viewport, &ok)
  sum += u.x + p.z
  // Fields of the unions are visible from Swift.
  var m = GLKMatrix4Identity
  m.m30 = 4
  sum += m.m30 + v.x + GLKVector4Make(0, 0, 0, 1).w + q.w + q.v.z
  return sum
}
_ = useMath()
