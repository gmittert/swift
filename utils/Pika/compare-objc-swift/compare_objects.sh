cp ~/projects/swift-extension/prod-5-2/original/ShareExtension\#iphoneos-arm64-LinkMap.txt Inputs-Orig/
cp ~/projects/swift-extension/prod-5-2/original/FBComposerDeferredFrameworkBinaryWithExportedSymbolsCleaned#iphoneos-arm64-LinkMap.txt Inputs-Orig/FBComposerDeferredFramework-LinkMap.txt
LC_CTYPE=C && LANG=C && sed -i t 's/libFBComposerLiteUIRootSwiftPlugin--288871838/libFBComposerLiteUIRootSwiftPlugin/g' Inputs-Orig/ShareExtension\#iphoneos-arm64-LinkMap.txt
LC_CTYPE=C && LANG=C && sed -i t 's/libFBComposerLiteUIRootPlugin-924888695/libFBComposerLiteUIRootPlugin/g' Inputs-Orig/ShareExtension\#iphoneos-arm64-LinkMap.txt
LC_CTYPE=C && LANG=C && sed -i t 's/libFBComposerGIFIntegrationPlugin--721104357/libFBComposerGIFIntegrationPlugin/g' Inputs-Orig/FBComposerDeferredFramework-LinkMap.txt
grep -E 'libFBComposerLiteDestination-|libFBComposerLiteUIRootPlugin-|libFBComposerLiteStoriesPreview-|libFBComposerLiteDestinationSwift-|libFBComposerLiteUIRootSwift-|libFBComposerLiteStoriesPreviewSwift-' Inputs-Orig/ShareExtension\#iphoneos-arm64-LinkMap.txt
grep -E 'libFBComposerGIFIntegrationPlugin-|libFBComposerGIFAttachmentKit-|libFBComposerGIFIntegrationPluginSwift-|libFBComposerGIFAttachmentKitSwift-' Inputs-Orig/FBComposerDeferredFramework-LinkMap.txt

buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/ShareExtension\#iphoneos-arm64-LinkMap.txt FBComposerLiteDestinationSwift &> Inputs-Orig/second.linkmap.swift
buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/ShareExtension\#iphoneos-arm64-LinkMap.txt FBComposerLiteUIRootSwift &> Inputs-Orig/first.linkmap.swift
buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/ShareExtension\#iphoneos-arm64-LinkMap.txt FBComposerLiteStoriesPreviewSwift &> Inputs-Orig/third.linkmap.swift
buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/ShareExtension\#iphoneos-arm64-LinkMap.txt FBComposerLiteDestination &> Inputs-Orig/second.linkmap.objc
buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/ShareExtension\#iphoneos-arm64-LinkMap.txt FBComposerLiteUIRootPlugin &> Inputs-Orig/first.linkmap.objc
buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/ShareExtension\#iphoneos-arm64-LinkMap.txt FBComposerLiteStoriesPreview &> Inputs-Orig/third.linkmap.objc

buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/FBComposerDeferredFramework-LinkMap.txt FBComposerGIFIntegrationPlugin &> Inputs-Orig/defer-1.linkmap.objc
buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/FBComposerDeferredFramework-LinkMap.txt FBComposerGIFAttachmentKit &> Inputs-Orig/defer-2.linkmap.objc
buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/FBComposerDeferredFramework-LinkMap.txt FBComposerGIFIntegrationPluginSwift &> Inputs-Orig/defer-1.linkmap.swift
buck run //fbobjc/Tools/appsize:compare_objects -- Inputs-Orig/FBComposerDeferredFramework-LinkMap.txt FBComposerGIFAttachmentKitSwift &> Inputs-Orig/defer-2.linkmap.swift

grep TOTAL Inputs-Orig/*linkmap*
