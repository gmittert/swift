pushd ~/fbsource/fbobjc
~/local/pika11/external/swift/utils/Pika/compare-objc-swift/compare_objects.sh
popd

echo "amended calculation on original set up"
echo "for share extension"
rm -rf Inputs
mkdir -p Inputs
# only copy files related to share extension
cp ~/fbsource/fbobjc/Inputs-Orig/* Inputs/
rm Inputs/*defer*
# need to copy objc.list or objc2.list to Inputs
cp ~/projects/swift-extension/prod-5-2/original/objc.list objc.filelist
sed -i t 's/^/\/Users\/mren\/projects\/swift-extension\/prod-5-2\/original\//g' objc.filelist
# find ~/projects/swift-extension/prod-5-2/original/ -name "*thinlto.o" &> objc.filelist
find ~/projects/swift-extension/prod-5-2/original/ -name "*Swift.o" | grep -v GIF &> swift.filelist
wc objc.filelist
wc swift.filelist
./amend-linkmap.sh

echo "for ComposerDeferred"
rm -rf Inputs
mkdir -p Inputs
# only copy files related to Composer
cp ~/fbsource/fbobjc/Inputs-Orig/*defer* Inputs/
# need to copy objc.list or objc2.list to Inputs
cp ~/projects/swift-extension/prod-5-2/original/objc2.list objc.filelist
sed -i t 's/^/\/Users\/mren\/projects\/swift-extension\/prod-5-2\/original\//g' objc.filelist
# find ~/projects/swift-extension/prod-5-2/original/ -name "*thinlto.o" &> objc.filelist
find ~/projects/swift-extension/prod-5-2/original/ -name "*Swift.o" | grep -E 'GIFAttachment|GIFIntegration' &> swift.filelist
wc objc.filelist
wc swift.filelist
./amend-linkmap.sh
 
echo "for FBComposerGIFAttachmentKit"
rm -rf Inputs
mkdir -p Inputs
# only copy files related to Composer
cp ~/fbsource/fbobjc/Inputs-Orig/*defer-2* Inputs/
grep GIFAttachment ~/projects/swift-extension/prod-5-2/original/FBComposerDeferredFramework-devirt-info | grep Mapping | awk '{print $3}' &> objc.filelist
sed -i t 's/$/.arm64.thinlto.o/g' objc.filelist
sed -i t 's/^/\/Users\/mren\/projects\/swift-extension\/prod-5-2\/original\//g' objc.filelist
# find ~/projects/swift-extension/prod-5-2/original/ -name "*thinlto.o" &> objc.filelist
find ~/projects/swift-extension/prod-5-2/original/ -name "*Swift.o" | grep GIFAttachment &> swift.filelist
wc objc.filelist
wc swift.filelist
./amend-linkmap.sh

echo "for FBComposerGIFIntegrationPlugin"
rm -rf Inputs
mkdir -p Inputs
# only copy files related to Composer
cp ~/fbsource/fbobjc/Inputs-Orig/*defer-1* Inputs/
grep GIFIntegration ~/projects/swift-extension/prod-5-2/original/FBComposerDeferredFramework-devirt-info | grep Mapping | awk '{print $3}' &> objc.filelist
sed -i t 's/$/.arm64.thinlto.o/g' objc.filelist
sed -i t 's/^/\/Users\/mren\/projects\/swift-extension\/prod-5-2\/original\//g' objc.filelist
# find ~/projects/swift-extension/prod-5-2/original/ -name "*thinlto.o" &> objc.filelist
find ~/projects/swift-extension/prod-5-2/original/ -name "*Swift.o" | grep GIFIntegration &> swift.filelist
wc objc.filelist
wc swift.filelist
./amend-linkmap.sh
