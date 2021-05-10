// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

import Foundation

private let kBottomSheetVersion = "2"
private let kFeedComposerVersion = "1"
private let kSourceType = "swift"

@objc(FBSwiftComposerLiteRootViewController)
public final class FBComposerLiteRootViewController: NSObject {
  private var _extensionController: FBComposerLiteExtensionController

  @objc(initWithExtensionController:)
  public init(extensionController: FBComposerLiteExtensionController) {
    _extensionController = extensionController
    super.init()
  }

}

