import subprocess, os

def configureDoxyfile(input_dir, output_dir):
    with open('Doxyfile.in', 'r') as file :
        filedata = file.read()
 
    filedata = filedata.replace('@DOXYGEN_INPUT_DIR@', input_dir)
    filedata = filedata.replace('@DOXYGEN_OUTPUT_DIR@', output_dir)
 
    with open('Doxyfile', 'w') as file:
        file.write(filedata)

read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

extensions = ["breathe"]
breathe_projects = {}
if read_the_docs_build:
    input_dir = '../'
    output_dir = 'dox'
    configureDoxyfile(input_dir, output_dir)
    subprocess.call('doxygen', shell=True)
    breathe_projects['udho'] = output_dir + '/xml'
    