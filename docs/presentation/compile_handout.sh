#!/bin/zsh
mkdir intermediate; 

lualatex --shell-escape --output-directory=./intermediate presentation.tex \
&& lualatex --shell-escape --output-directory=./intermediate presentation.tex \
&& mv ./intermediate/presentation.pdf ./handout.pdf;

evince ./handout.pdf;
