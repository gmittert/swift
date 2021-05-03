############################
cp buck-out/gen/afdfe660/fbobjc/Apps/Wilde/Facebook/ShareExtensionBinary#iphoneos-arm64-LinkMap.txt /home/mren/projects/swift-extension/$1
# save link map after processing
cp fbobjc/build_output/LinkMap/ShareExtension#iphoneos-arm64-LinkMap.txt /home/mren/projects/swift-extension/$1

cp buck-out/gen/afdfe660/fbobjc/Tools/dependency_aggregator/result/FBComposerDeferredFrameworkBinaryWithExportedSymbolsCleaned#iphoneos-arm64,shared/FBComposerDeferredFramework-LinkMap.txt /home/mren/projects/swift-extension/$1
cp fbobjc/build_output/LinkMap/FBComposerDeferredFrameworkBinaryWithExportedSymbolsCleaned#iphoneos-arm64-LinkMap.txt /home/mren/projects/swift-extension/$1
############################
# swift .o files
find buck-out/ -name "*Swift.o" | grep FBComposerLiteExtension &> ttt
sed -i 's/^/cp /g' ttt
sed -i 's/$/ \/home\/mren\/projects\/swift-extension\/\$1/g' ttt
mv ttt tmp-backup.sh
chmod a+x tmp-backup.sh
./tmp-backup.sh $1

# mapping from thinlto object files to source modules
cp buck-out/gen/afdfe660/fbobjc/Apps/Wilde/Facebook/ShareExtensionBinary#iphoneos-arm64-lto//devirt-info/ShareExtension /home/mren/projects/swift-extension/$1/ShareExtension-devirt-info

grep -E 'libFBComposerGIFIntegrationPlugin|libFBComposerGIFAttachmentKit|libFBComposerLiteDestination|libFBComposerLiteUIRootPlugin|libFBComposerLiteStoriesPreview' buck-out/gen/afdfe660/fbobjc/Apps/Wilde/Facebook/ShareExtensionBinary#iphoneos-arm64-lto//devirt-info/ShareExtension | grep "# Mapping" | awk '{print $3}' | sort &> ttt
sed -i 's/$/.arm64.thinlto.o/g' ttt
cp ttt /home/mren/projects/swift-extension/$1/objc.list
sed -i 's/^/cp buck-out\/gen\/afdfe660\/fbobjc\/Apps\/Wilde\/Facebook\/ShareExtensionBinary#iphoneos-arm64-lto\//g' ttt
sed -i 's/$/ \/home\/mren\/projects\/swift-extension\/\$1/g' ttt
mv ttt tmp-backup.sh
chmod a+x tmp-backup.sh
./tmp-backup.sh $1

# handle FBComposerDeferredFrameworkBinary
# swift .o files
find buck-out/ -name "*Swift.o" | grep -E 'FBComposerGIFIntegration|FBComposerGIFAttachment|FBComposerGIFPicker' &> ttt
sed -i 's/^/cp /g' ttt
sed -i 's/$/ \/home\/mren\/projects\/swift-extension\/\$1/g' ttt
mv ttt tmp-backup.sh
chmod a+x tmp-backup.sh
./tmp-backup.sh $1

# mapping from thinlto object files to source modules
cp buck-out/gen/afdfe660/fbobjc/Tools/dependency_aggregator/result/FBComposerDeferredFrameworkBinaryWithExportedSymbolsCleaned#iphoneos-arm64,shared/FBComposerDeferredFramework-lto/devirt-info/FBComposerDeferredFramework /home/mren/projects/swift-extension/$1/FBComposerDeferredFramework-devirt-info

grep -E 'libFBComposerGIFIntegrationPlugin|libFBComposerGIFAttachmentKit|libFBComposerLiteDestination|libFBComposerLiteUIRootPlugin|libFBComposerLiteStoriesPreview' buck-out/gen/afdfe660/fbobjc/Tools/dependency_aggregator/result/FBComposerDeferredFrameworkBinaryWithExportedSymbolsCleaned#iphoneos-arm64,shared/FBComposerDeferredFramework-lto/devirt-info/FBComposerDeferredFramework | grep "# Mapping" | awk '{print $3}' | sort &> ttt
sed -i 's/$/.arm64.thinlto.o/g' ttt
cp ttt /home/mren/projects/swift-extension/$1/objc2.list
sed -i 's/^/cp buck-out\/gen\/afdfe660\/fbobjc\/Tools\/dependency_aggregator\/result\/FBComposerDeferredFrameworkBinaryWithExportedSymbolsCleaned#iphoneos-arm64,shared\/FBComposerDeferredFramework-lto\//g' ttt
sed -i 's/$/ \/home\/mren\/projects\/swift-extension\/\$1/g' ttt
mv ttt tmp-backup2.sh
chmod a+x tmp-backup2.sh
./tmp-backup2.sh $1

#######################
cp buck-out/bin/afdfe660/fbobjc/Apps/Wilde/Facebook/ShareExtensionBinary#binary,iphoneos-arm64/filelist.txt /home/mren/projects/swift-extension/$1
cp buck-out/bin/afdfe660/fbobjc/Tools/dependency_aggregator/result/FBComposerDeferredFrameworkBinaryWithExportedSymbolsCleaned#iphoneos-arm64,shared/filelist.txt /home/mren/projects/swift-extension/$1/filelist-deferred.txt

cp fbobjc/build_output/*ipa /home/mren/projects/swift-extension/$1
