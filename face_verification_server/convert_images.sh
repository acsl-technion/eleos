#!/bin/bash
img_convert() {
    echo "processing directory: $1"
    for d in ./colorferet/$1/data/images/*; do
        if [ -d "$d" ]; then
            echo $d
            for f in "$d"/*; do
                cp $f ./images_raw/
                bzip2 -d ./images_raw/$(basename "$f")
                ppm=$(basename "$f" .bz2)
                convert ./images_raw/"$ppm" -resize "512x512^" -gravity center -crop 512x512+0+0 +repage -depth 8 gray:images_raw/$(basename "$ppm" .ppm).dat
            done
        fi
    done
    \rm -f ./images_raw/*.bz2
    find . -name "*.ppm" -print0 | xargs -0 rm
    return 0
}

mkdir -p ./images_raw/
img_convert "dvd1"
img_convert "dvd2"
