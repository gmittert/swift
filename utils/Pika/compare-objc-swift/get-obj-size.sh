#!/bin/bash
 
rm -f Outputs/tmp-size.combined
touch Outputs/tmp-size.combined
while read val; do
  size -m $val &> Outputs/ttt
  cat Outputs/tmp-size.combined Outputs/ttt &> Outputs/ttt2
  mv Outputs/ttt2 Outputs/tmp-size.combined
done <Outputs/filelist.txt
