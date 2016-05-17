#!/bin/bash

mkdir -p ~/.vim/syntax
cat ./filetype.vim >> ~/.vim/filetype.vim
cp ./jfr_log.vim ~/.vim/syntax/.
