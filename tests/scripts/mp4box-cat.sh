

test_cat()
{
test_begin "mp4box-cat-$1"
if [ "$test_skip" = 1 ] ; then
return
fi

mp4file="$TEMP_DIR/test-addcat.mp4"
do_test "$MP4BOX -add $2 -cat $2 -new $mp4file" "addcat"
do_hash_test $mp4file "addcat"

if [ $3 != 0 ] ; then

mp4file="$TEMP_DIR/base.mp4"
do_test "$MP4BOX -add $2 -new $mp4file" "add"
do_hash_test $mp4file "add"
catfile="$TEMP_DIR/test-add-cat.mp4"
do_test "$MP4BOX -cat $2 $mp4file -out $catfile" "cat"
do_hash_test $catfile "cat"

catfile="$TEMP_DIR/test-catmp4.mp4"
do_test "$MP4BOX -cat $mp4file $mp4file -out $catfile" "catmp4"
do_hash_test $catfile "catmp4"

plfile="$TEMP_DIR/pl.txt"
echo $2 > $plfile
echo $2 >> $plfile

catfile="$TEMP_DIR/test-catpl.mp4"
do_test "$MP4BOX -catpl $plfile -new $catfile" "catpl"
do_hash_test $catfile "catpl"

catfile="$TEMP_DIR/test-catplmp4.mp4"
do_test "$MP4BOX -catpl $plfile $mp4file -out $catfile" "catplmp4"
do_hash_test $catfile "catplmp4"

fi

test_end
}

test_cat "avc" $MEDIA_DIR/auxiliary_files/enst_video.h264 1
test_cat "hevc" $MEDIA_DIR/auxiliary_files/counter.hvc 0
test_cat "aac" $MEDIA_DIR/auxiliary_files/enst_audio.aac 0
test_cat "srt" $MEDIA_DIR/auxiliary_files/subtitle.srt 0

