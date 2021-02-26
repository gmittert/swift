# inputs: xxx.linkmap.objc xxx.linkmap.swift
# inputs: object files, objc.filelist, swift.filelist
# -- object files are listed in objc.filelist and swift.filelist

mkdir -p Outputs
cp objc.filelist Outputs/filelist.txt
./get-obj-size.sh
mv Outputs/tmp-size.combined ttt
python ./merge-section-obj.py ttt &> objc.combined

cp swift.filelist Outputs/filelist.txt
./get-obj-size.sh
mv Outputs/tmp-size.combined ttt
python ./merge-section-obj.py ttt &> swift.combined
rm Outputs/filelist.txt

# accumulate size from linker map for the three modules
# one extra linkmap file with monoLTO
if [ "$1" == "mono" ]
then
cat Inputs/second.linkmap.swift Inputs/first.linkmap.swift Inputs/third.linkmap.swift Inputs/allswift.linkmap.swift | grep ':' | grep -v "PARSING" | grep -v "CREATING" | grep -v "BUILDING" &> ttt.swift
else
cat Inputs/second.linkmap.swift Inputs/first.linkmap.swift Inputs/third.linkmap.swift | grep ':' | grep -v "PARSING" | grep -v "CREATING" | grep -v "BUILDING" &> ttt.swift
fi
cat Inputs/second.linkmap.objc Inputs/first.linkmap.objc Inputs/third.linkmap.objc | grep ':' | grep -v "PARSING" | grep -v "CREATING" | grep -v "BUILDING" &> ttt.objc
grep ',' ttt.objc &> tmp.objc
grep ',' ttt.swift &> tmp.swift
python merge-section-linkmap.py tmp.swift &> combined.lm.swift
python merge-section-linkmap.py tmp.objc &> combined.lm.objc

echo "--------------------------"
echo "swift vs objc linkmap"
python ./compare-section.py combined.lm.swift combined.lm.objc &> ttt
echo "related sections in linker map"
grep -E 'objc_ivarname|objc_methtype|objc_classname|objc_methname|objc_selref|__cstring' ttt | awk '{sum+=$2; sum2+=$3} END {print sum, sum2}'
echo "sum of all sections in linker map"
grep -v odrcov ttt | awk '{sum+=$2; sum2+=$3} END {print sum, sum2, sum/sum2}'
echo "remove related sections in linker map"
grep -v odrcov ttt | grep -v objc_ivarname | grep -v objc_methtype | grep -v objc_classname | grep -v objc_methname | grep -v objc_selref | grep -v __cstring | awk '{sum+=$2; sum2+=$3} END {print sum, sum2}' &> other.lm.tmp
cat other.lm.tmp

# check object files
python ./compare-section.py swift.combined objc.combined &> ttt2
echo "related sections in object files"
grep -E 'objc_ivarname|objc_methtype|objc_classname|objc_methname|objc_selref|__cstring' ttt2 | awk '{sum+=$2; sum2+=$3} END {print sum, sum2}' &> related.obj.tmp
cat related.obj.tmp

echo "amended sections in linker map"
# other.lm.tmp + related.obj.tmp
paste other.lm.tmp related.obj.tmp | awk '{print $1+$3, $2+$4, ($1+$3)/($2+$4)}'

echo "section comparison from linker map"
awk '{print $1, $2, $3, $3-$2}' ttt | sort -n -k 4 | awk '$4!=0 {print $1,$2,$3,$4}' | grep -v odrcov | grep __text

rm *tmp* ttt* *combined*
