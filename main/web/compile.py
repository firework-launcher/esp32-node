import shutil
import os
import jinja2

os.makedirs('rendered', exist_ok=True)

folder = 'rendered'
for filename in os.listdir(folder):
    file_path = os.path.join(folder, filename)
    try:
        if os.path.isfile(file_path) or os.path.islink(file_path):
            os.unlink(file_path)
        elif os.path.isdir(file_path):
            shutil.rmtree(file_path)
    except Exception as e:
        print('Failed to delete %s. Reason: %s' % (file_path, e))

shutil.copytree('static', 'rendered', dirs_exist_ok=True)

jinja_env = jinja2.Environment(loader=jinja2.FileSystemLoader('templates'))

def render(file):
    file = file.replace('templates/', '')
    if not file == 'base.html':
        template = jinja_env.get_template(file)
        data = template.render(
            name='Firework Launcher',
            page=file
        )
        os.makedirs(os.path.normpath(os.path.join('rendered', file, '..')), exist_ok=True)
        rendered_file = open(os.path.join('rendered', file), 'x')
        rendered_file.write(data)
        rendered_file.close()

def render_files(path):
    for file in os.listdir(path):
        if os.path.isdir(os.path.join(path, file)):
            render_files(os.path.join(path, file))
        else:
            render(os.path.join(path, file))

render_files('templates')
