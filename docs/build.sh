python -m pip install --upgrade --no-cache-dir pip --user
python -m pip install --exists-action=w --no-cache-dir -r requirements.txt --user
sphinx-build -T -d _build/doctrees-readthedocs -D language=en . _build/html
sphinx-build -T -d _build/doctrees-readthedocssinglehtmllocalmedia -D language=en . _build/localmedia
#sphinx-build -b latex -D language=en -d _build/doctrees . _build/latex
#cd _build/latex
#make
#mv python.pdf udho.pdf
#cd ../..
