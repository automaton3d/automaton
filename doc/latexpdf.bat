cd /d E:\automaton\doc
git fetch origin
git reset --hard origin/devin/polar-phase
git clean -fd
pdflatex manuscript.tex
biber manuscript
pdflatex manuscript.tex
pdflatex manuscript.tex
manuscript.pdf
