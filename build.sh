set -xe

if [[ $1 == "raylib" ]] 
then
    cc -lm -g -lraylib -std=c11 main.c -o graphics
else 
    cc -lm -g -std=c11 main.c -o graphics
fi

./graphics
