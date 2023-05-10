#!/bin/zsh
mkdir intermediate; 

lualatex --output-directory=./intermediate main.tex \
&& biber ./intermediate/main \
&& makeglossaries -d ./intermediate main \
&& lualatex --output-directory=./intermediate main.tex \
&& mv ./intermediate/main.pdf ./main.pdf;

evince ./main.pdf;
