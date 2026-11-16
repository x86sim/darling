import Foundation
import UniformTypeIdentifiers

// UniformTypeIdentifiers imports as a Clang module from the SDK: the Objective-C UTType is UTTypeReference in Swift,
// its factory methods are initializers, and the constants import by their Objective-C names.
let reference = UTTypeReference("public.png")
let constant: UTTypeReference = UTTypePNG
if reference?.identifier == "public.png" && constant.identifier == "public.png"
    && reference?.conforms(to: UTTypeImage) == true && reference?.isPublic == true
    && UTTypeReference(exportedAs: "org.example.document", conformingTo: UTTypeJSON).conforms(to: UTTypeJSON) {
    print("ALL PASSED")
} else {
    print("FAIL: \(String(describing: reference)) \(constant)")
    exit(1)
}
