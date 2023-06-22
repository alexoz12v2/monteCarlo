#!/bin/zsh
mkdir intermediate; 

lualatex --shell-escape --output-directory=./intermediate main.tex \
&& biber ./intermediate/main \
&& makeglossaries -d ./intermediate main \
&& lualatex --shell-escape --output-directory=./intermediate main.tex \
&& mv ./intermediate/main.pdf ./main.pdf;

evince ./main.pdf;
